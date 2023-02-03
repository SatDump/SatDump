#include "generic_correlator.h"
#include "common/dsp/buffer.h"
#include "rotation.h"
#include "resources.h"
#include "logger.h"

CorrelatorGeneric::CorrelatorGeneric(dsp::constellation_type_t mod, std::vector<uint8_t> syncword, int max_frm_size) : d_modulation(mod)
{
    converted_buffer = dsp::create_volk_buffer<float>(max_frm_size * 2);
    syncword_length = syncword.size();

    // Generate syncwords
    if (d_modulation == dsp::BPSK)
    {
        syncwords.resize(2);
        for (int i = 0; i < 2; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }
        rotate_float_buf(syncwords[1].data(), syncword_length, 180);
    }
    else if (d_modulation == dsp::QPSK)
    {
        syncwords.resize(4);
        for (int i = 0; i < 4; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }
        rotate_float_buf(syncwords[1].data(), syncword_length, 90);
        rotate_float_buf(syncwords[2].data(), syncword_length, 180);
        rotate_float_buf(syncwords[3].data(), syncword_length, 270);
    }
    else if (d_modulation == dsp::OQPSK)
    {
        syncwords.resize(8);

        for (int i = 0; i < 4; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }

        // Syncword if we delayed the wrong phase
        uint8_t last_q_oqpsk = 0;
        for (int i = 0; i < syncword_length / 2; i++)
        {
            uint8_t back = syncword[i * 2 + 1];
            syncword[i * 2 + 1] = last_q_oqpsk;
            last_q_oqpsk = back;
        }

        for (int i = 4; i < 8; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }

        rotate_float_buf(syncwords[1].data(), syncword_length, 90);
        rotate_float_buf(syncwords[2].data(), syncword_length, 180);
        rotate_float_buf(syncwords[3].data(), syncword_length, 270);

        rotate_float_buf(syncwords[5].data(), syncword_length, 90);
        rotate_float_buf(syncwords[6].data(), syncword_length, 180);
        rotate_float_buf(syncwords[7].data(), syncword_length, 270);
    }

#ifdef USE_OPENCL1
    try
    {
        corro = new float[max_frm_size];
        matcho = new int[max_frm_size];

        std::vector<float> full_syncs(syncwords.size() * syncword_length);

        for (int i = 0; i < (int)syncwords.size(); i++)
            for (int x = 0; x < syncword_length; x++)
                full_syncs[i * syncword_length + x] = syncwords[i][x];

        satdump::opencl::setupOCLContext();

        corr_program = satdump::opencl::buildCLKernel(resources::getResourcePath("opencl/generic_correlator.cl"));

        cl_int err = 0;

        buffer_syncs = clCreateBuffer(satdump::opencl::ocl_context, CL_MEM_READ_WRITE, sizeof(float) * syncword_length * syncwords.size(), NULL, &err);
        if (err != CL_SUCCESS)
            throw std::runtime_error("Couldn't load buffer_map!");

        buffer_input = clCreateBuffer(satdump::opencl::ocl_context, CL_MEM_READ_WRITE, sizeof(float) * max_frm_size, NULL, &err);
        if (err != CL_SUCCESS)
            throw std::runtime_error("Couldn't load buffer_map!");

        buffer_corrs = clCreateBuffer(satdump::opencl::ocl_context, CL_MEM_READ_WRITE, sizeof(float) * max_frm_size, NULL, &err);
        if (err != CL_SUCCESS)
            throw std::runtime_error("Couldn't load buffer_map!");

        buffer_matches = clCreateBuffer(satdump::opencl::ocl_context, CL_MEM_READ_WRITE, sizeof(int) * max_frm_size, NULL, &err);
        if (err != CL_SUCCESS)
            throw std::runtime_error("Couldn't load buffer_map!");

        buffer_nsyncs = clCreateBuffer(satdump::opencl::ocl_context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err);
        if (err != CL_SUCCESS)
            throw std::runtime_error("Couldn't load buffer_map!");

        buffer_syncsize = clCreateBuffer(satdump::opencl::ocl_context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err);
        if (err != CL_SUCCESS)
            throw std::runtime_error("Couldn't load buffer_map!");

        cl_queue = clCreateCommandQueue(satdump::opencl::ocl_context, satdump::opencl::ocl_device, 0, &err);

        clEnqueueWriteBuffer(cl_queue, buffer_syncs, true, 0, sizeof(float) * syncword_length * syncwords.size(), full_syncs.data(), 0, NULL, NULL);

        int nsync = syncwords.size();
        clEnqueueWriteBuffer(cl_queue, buffer_nsyncs, true, 0, sizeof(float), &nsync, 0, NULL, NULL);
        clEnqueueWriteBuffer(cl_queue, buffer_syncsize, true, 0, sizeof(float), &syncword_length, 0, NULL, NULL);

        corr_kernel = clCreateKernel(corr_program, "correlate", &err);
        clSetKernelArg(corr_kernel, 0, sizeof(cl_mem), &buffer_syncs);
        clSetKernelArg(corr_kernel, 1, sizeof(cl_mem), &buffer_input);
        clSetKernelArg(corr_kernel, 2, sizeof(cl_mem), &buffer_corrs);
        clSetKernelArg(corr_kernel, 3, sizeof(cl_mem), &buffer_matches);
        clSetKernelArg(corr_kernel, 4, sizeof(cl_mem), &buffer_nsyncs);
        clSetKernelArg(corr_kernel, 5, sizeof(cl_mem), &buffer_syncsize);

        use_gpu = true;
        logger->trace("Correlator will use GPU!");
    }
    catch (std::exception &e)
    {
        logger->warn("Correlator can't use GPU : {:s}", e.what());
    }
#endif
}

void CorrelatorGeneric::rotate_float_buf(float *buf, int size, float rot_deg)
{
    float phase = rot_deg * 0.01745329;
    complex_t *cc = (complex_t *)buf;
    for (int i = 0; i < size / 2; i++)
        cc[i] = cc[i] * complex_t(cosf(phase), sinf(phase));
}

void CorrelatorGeneric::modulate_soft(float *buf, uint8_t *bit, int size)
{
    for (int i = 0; i < size; i++)
        buf[i] = bit[i] ? 1.0f : -1.0f;
}

CorrelatorGeneric::~CorrelatorGeneric()
{
    volk_free(converted_buffer);

#ifdef USE_OPENCL1
    delete[] corro;
    delete[] matcho;

    if (use_gpu)
    {
        clReleaseProgram(corr_program);
        clReleaseKernel(corr_kernel);

        clReleaseMemObject(buffer_syncs);
        clReleaseMemObject(buffer_input);
        clReleaseMemObject(buffer_corrs);
        clReleaseMemObject(buffer_matches);
        clReleaseMemObject(buffer_nsyncs);
        clReleaseMemObject(buffer_syncsize);

        clReleaseCommandQueue(cl_queue);
    }
#endif
}

int CorrelatorGeneric::correlate(int8_t *soft_input, phase_t &phase, bool &swap, float &cor, int length)
{
    volk_8i_s32f_convert_32f(converted_buffer, soft_input, 127 / 2, length);

    float corr = 0;
    int position = 0, best_sync = 0;

#ifdef USE_OPENCL1
    if (use_gpu)
    {
        clEnqueueWriteBuffer(cl_queue, buffer_input, true, 0, sizeof(float) * length, converted_buffer, 0, NULL, NULL);

        size_t total_wg_size = length - syncword_length;
        if (clEnqueueNDRangeKernel(cl_queue, corr_kernel, 1, NULL, &total_wg_size, NULL, 0, NULL, NULL) != CL_SUCCESS)
            throw std::runtime_error("Couldn't clEnqueueNDRangeKernel!");

        clEnqueueReadBuffer(cl_queue, buffer_corrs, true, 0, sizeof(float) * length, corro, 0, NULL, NULL);
        clEnqueueReadBuffer(cl_queue, buffer_matches, true, 0, sizeof(int) * length, matcho, 0, NULL, NULL);

        cor = 0;
        for (int i = 0; i < length - syncword_length; i++)
        {
            if (corro[i] > cor)
            {
                cor = corro[i];
                best_sync = matcho[i];
                position = i;
            }
        }
    }
    else
#endif
    {
        cor = 0;

        for (int i = 0; i < length - syncword_length; i++)
        {
            for (int s = 0; s < (int)syncwords.size(); s++)
            {
                volk_32f_x2_dot_prod_32f(&corr, &converted_buffer[i], syncwords[s].data(), syncword_length);

                if (corr > cor)
                {
                    cor = corr;
                    best_sync = s;
                    position = i;
                }
            }
        }
    }

    if (d_modulation == dsp::BPSK)
    {
        phase = best_sync ? PHASE_180 : PHASE_0;
        swap = false;
    }
    else if (d_modulation == dsp::QPSK || d_modulation == dsp::OQPSK)
    {
        phase = (phase_t)(best_sync % 4);
        swap = (best_sync / 4);
    }

    return position;
}