#include "xrit_channel_processor.h"
#include "core/exception.h"
#include "get_img.h"
#include "logger.h"
#include "xrit/fy4/fy4_headers.h"
#include "xrit/fy4/segment_decoder.h"
#include "xrit/gk2a/segment_decoder.h"
#include "xrit/goes/segment_decoder.h"
#include "xrit/himawari/segment_decoder.h"
#include "xrit/identify.h"
#include "xrit/msg/segment_decoder.h"

namespace satdump
{
    namespace xrit
    {
        void XRITChannelProcessor::push(xrit::XRITFileInfo &finfo, XRITFile &file)
        {
            if (!file.hasHeader<ImageStructureRecord>())
            {
                if (!saveMeta(finfo, file))
                    logger->error("Not image data, and didn't process as metadata!");
                return;
            }

            // China being non-standard.....
            if (finfo.type == XRIT_FY4_AGRI)
            {
                fy4::ImageInformationRecord image_structure_record = file.getHeader<fy4::ImageInformationRecord>();
                logger->debug("This is image data (FY-4x!). Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));
            }
            else
            {
                ImageStructureRecord image_structure_record = file.getHeader<ImageStructureRecord>();
                logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));
            }

            if (finfo.seg_groupid.size() == 0)
            {
                auto img = getImageFromXRITFile(finfo.type, file);
                saveImg(finfo, img);
            }
            else
            {
                // Get segmented decoder
                std::string seg_dec_id = finfo.channel;
                if (segmented_decoders.count(seg_dec_id) == 0)
                    segmented_decoders.emplace(seg_dec_id, nullptr);
                auto &segDecoder = segmented_decoders[seg_dec_id];

                // Get image status
                ui_img_mtx.lock();
                if (all_wip_images.count(seg_dec_id) == 0)
                    all_wip_images.insert({seg_dec_id, std::make_unique<wip_images>()});
                std::unique_ptr<wip_images> &wip_img = all_wip_images[seg_dec_id];
                ui_img_mtx.unlock();

                if (!segDecoder || segDecoder->info.seg_groupid != finfo.seg_groupid || segDecoder->image.size() == 0)
                {
                    if (segDecoder && segDecoder->hasData())
                    {
                        wip_img->imageStatus = SAVING;
                        saveImg(segDecoder->info, segDecoder->image);
                    }

                    if (finfo.type == xrit::XRIT_ELEKTRO_MSUGS || finfo.type == xrit::XRIT_MSG_SEVIRI)
                        segDecoder = std::make_shared<MSGSegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_GOES_ABI || finfo.type == xrit::XRIT_GOES_HIMAWARI_AHI || finfo.type == XRIT_GOESN_IMAGER)
                        segDecoder = std::make_shared<GOESSegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_GK2A_AMI)
                        segDecoder = std::make_shared<GK2ASegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_HIMAWARI_AHI)
                        segDecoder = std::make_shared<HimawariSegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_FY4_AGRI)
                        segDecoder = std::make_shared<FY4xSegmentedImageDecoder>(file);
                    else
                        throw satdump_exception("Unsupported segmented file type!");

                    segDecoder->info = finfo;
                    wip_img->imageStatus = RECEIVING;
                }

                segDecoder->pushSegment(file);

                // If the UI is active, update texture
                ui_img_mtx.lock();
                if (wip_img->textureID > 0)
                {
                    // Downscale image
                    image::Image imageScaled = segDecoder->image.resize_to(wip_img->img_width, wip_img->img_height);
                    image::image_to_rgba(imageScaled, wip_img->textureBuffer);
                    wip_img->hasToUpdate = true;
                }
                ui_img_mtx.unlock();

                if (segDecoder->isComplete())
                {
                    wip_img->imageStatus = SAVING;
                    saveImg(segDecoder->info, segDecoder->image);
                    segDecoder->reset();
                    wip_img->imageStatus = IDLE;
                }
            }
        }

        void XRITChannelProcessor::flush()
        {
            // Free up texture memory!
            ui_img_mtx.lock();
            for (auto &seg : segmented_decoders)
            {
                auto &segDecoder = seg.second;
                if (segDecoder && segDecoder->hasData())
                {
                    saveImg(segDecoder->info, segDecoder->image);
                    segDecoder->reset();
                }
            }
            ui_img_mtx.unlock();
        }

        XRITChannelProcessor::~XRITChannelProcessor()
        {
            for (auto &decMap : all_wip_images)
                if (decMap.second->textureID > 0)
                    delete[] decMap.second->textureBuffer;
        }
    } // namespace xrit
} // namespace satdump