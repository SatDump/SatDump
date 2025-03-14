#include "image_background.h"
#include "logger.h"
/*#include "common/geodetic/geodetic_coordinates.h"
#include "common/projection/reprojector.h"
#include "common/projection/projs2/proj_json.h"

namespace image
{
	void remove_background(Image &img, nlohmann::json proj_cfg, float *progress)
	{
		size_t width = img.width();
		size_t height = img.height();

		satdump::reprojection::rescaleProjectionScalarsIfNeeded(proj_cfg, width, height);
		proj::projection_t product_proj;
		try
		{
			product_proj = proj_cfg;
		}
		catch (std::exception &)
		{
			logger->warn("Cannot remove background - Failed to get projection config");
			return;
		}
		if (proj::projection_setup(&product_proj))
		{
			// TODO: Support for warp?
			logger->warn("Cannot remove background - Failed to set up projection algorithm");
			return;
		}

		img.to_rgba();
		double dummy; // Hey! I resemble that remark!

		for (size_t y = 0; y < height; y++)
		{
			for (size_t x = 0; x < width; x++)
				if (proj::projection_perform_inv(&product_proj, x, y, &dummy, &dummy))
					for (int c = 0; c < img.channels(); c++)
						img.set(c, y * width + x, 0);

			if (progress != nullptr)
				*progress = (float)y / (float)height;
		}

		proj::projection_free(&product_proj);
	}
} TODOREWORK MAKE WORK AGAIN*/