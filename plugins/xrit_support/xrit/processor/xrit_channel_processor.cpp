#include "xrit_channel_processor.h"
#include "core/exception.h"
#include "get_img.h"
#include "logger.h"
#include "xrit/gk2a/segment_decoder.h"
#include "xrit/goes/segment_decoder.h"
#include "xrit/himawari/segment_decoder.h"
#include "xrit/identify.h"
#include "xrit/msg/segment_decoder.h"

namespace satdump
{
    namespace xrit
    {
        void XRITChannelProcessor::push(xrit::XRITFileInfo &finfo, ::lrit::LRITFile &file)
        {
            if (!file.hasHeader<::lrit::ImageStructureRecord>())
            {
                logger->error("Not image data!");
                return;
            }

            ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();
            logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

            // TODOREWORK MSG EPI/PRO

            if (finfo.seg_groupid.size() == 0)
            {
                auto img = getImageFromXRITFile(finfo.type, file);
                saveImg(finfo, img);
            }
            else
            {
                std::string seg_dec_id = finfo.channel;
                if (segmented_decoders.count(seg_dec_id) == 0)
                    segmented_decoders.emplace(seg_dec_id, nullptr);
                auto &segDecoder = segmented_decoders[seg_dec_id];

                if (!segDecoder || segDecoder->info.seg_groupid != finfo.seg_groupid)
                {
                    if (segDecoder && segDecoder->hasData())
                        saveImg(segDecoder->info, segDecoder->image);

                    if (finfo.type == xrit::XRIT_ELEKTRO_MSUGS || finfo.type == xrit::XRIT_MSG_SEVIRI)
                        segDecoder = std::make_shared<MSGSegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_GOES_ABI || finfo.type == xrit::XRIT_GOES_HIMAWARI_AHI || finfo.type == XRIT_GOESN_IMAGER)
                        segDecoder = std::make_shared<GOESSegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_GK2A_AMI)
                        segDecoder = std::make_shared<GK2ASegmentedImageDecoder>(file);
                    else if (finfo.type == xrit::XRIT_HIMAWARI_AHI)
                        segDecoder = std::make_shared<HimawariSegmentedImageDecoder>(file);
                    else
                        throw satdump_exception("Unsupported segmented file type!");

                    segDecoder->info = finfo;
                }

                segDecoder->pushSegment(file);

                if (segDecoder->isComplete())
                {
                    saveImg(segDecoder->info, segDecoder->image);
                    segDecoder->reset();
                }
            }
        }

        void XRITChannelProcessor::flush()
        {
            for (auto &seg : segmented_decoders)
            {
                auto &segDecoder = seg.second;
                if (segDecoder && segDecoder->hasData())
                {
                    saveImg(segDecoder->info, segDecoder->image);
                    segDecoder->reset();
                }
            }
        }
    } // namespace xrit
} // namespace satdump