/*
  OpenCL Kernel capable of reprojecting
  a mercator projection to an
  equirectangular one.

  This is faster on the GPU :-)
*/

struct merc_cfg {
  int image_height;
  int image_width;
  int actual_image_height;
  int actual_image_width;
  float scale_x;
  float scale_y;
};

void mercator_forward(struct merc_cfg *cfg, float lon, float lat, int *x,
                      int *y) {
  if (lat > 85.06f || lat < -85.06f) {
    *x = *y = -1;
    return;
  }

  float px = (lon / 180.0f) * (cfg->image_width * cfg->scale_x);
  float py = asinh(tan(lat / 57.29578f)) * (cfg->image_height * cfg->scale_y);

  *x = px + (cfg->image_width / 2);
  *y = cfg->image_height - (py + (cfg->image_height / 2));

  if (*x < 0 || *y < 0)
    *x = *y = -1;
  if (*x >= cfg->actual_image_width || *y >= cfg->actual_image_height)
    *x = *y = -1;
}

void kernel reproj_image_merc_to_equ(global ushort *source_img,
                                     global ushort *target_img,
                                     global int *img_sizes,
                                     global float *img_merc_settings,
                                     global float *img_equ_settings) {

  int id = get_global_id(0);
  int nthreads = get_global_size(0);

  // Image sizes
  int source_img_width = img_sizes[0];
  int source_img_height = img_sizes[1];
  int target_img_width = img_sizes[2];
  int target_img_height = img_sizes[3];
  int source_channels = img_sizes[4];
  int target_channels = img_sizes[5];

  // Merc settings
  struct merc_cfg merc_cfg;
  merc_cfg.image_height = img_merc_settings[0];
  merc_cfg.image_width = img_merc_settings[1];
  merc_cfg.actual_image_height = img_merc_settings[2];
  merc_cfg.actual_image_width = img_merc_settings[3];
  merc_cfg.scale_x = img_merc_settings[4];
  merc_cfg.scale_y = img_merc_settings[5];

  // Equirectangular settings
  float top_left_lat = img_equ_settings[0];
  float top_left_lon = img_equ_settings[1];
  float bottom_right_lat = img_equ_settings[2];
  float bottom_right_lon = img_equ_settings[3];
  float covered_lat = fabs(top_left_lat - bottom_right_lat);
  float covered_lon = fabs(top_left_lon - bottom_right_lon);
  float offset_lat = fabs(top_left_lat - 90);
  float offset_lon = fabs(top_left_lon + 180);

  size_t n = target_img_width * target_img_height;

  size_t ratio = (n / nthreads); // number of elements for each thread
  size_t start = ratio * id;
  size_t stop = ratio * (id + 1);

  float lon = 0, lat = 0;
  int xx = 0, yy = 0;

  for (size_t xy_ptr = start; xy_ptr < stop; xy_ptr++) {
    int x = (xy_ptr % target_img_width);
    int y = (xy_ptr / target_img_width);

    // Equ => Lat/Lon
    if (y < 0 || y >= target_img_height || x < 0 || x >= target_img_width)
      continue;

    lat = (y / (float)target_img_height) * covered_lat;
    lon = (x / (float)target_img_width) * covered_lon;

    lat += offset_lat;
    lon += offset_lon;

    lat = 180.0f - (lat + 90.0f);
    lon -= 180;

    if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon ||
        lon > bottom_right_lon)
      continue;

    // Lat/Lon => Mercator
    mercator_forward(&merc_cfg, lon, lat, &xx, &yy);
    if (xx == -1 || yy == -1 || xx >= (int)source_img_width || xx < 0 ||
        yy >= (int)source_img_height || yy < 0)
      continue;

    if (source_channels == 4)
      for (int c = 0; c < target_channels; c++)
        target_img[(target_img_width * target_img_height) * c +
                   y * target_img_width + x] =
            source_img[(source_img_width * source_img_height) * c +
                       yy * source_img_width + xx];
    else if (source_channels == 3)
      for (int c = 0; c < target_channels; c++)
        target_img[(target_img_width * target_img_height) * c +
                   y * target_img_width + x] =
            c == 3 ? 65535
                   : source_img[(source_img_width * source_img_height) * c +
                                yy * source_img_width + xx];
    else
      for (int c = 0; c < target_channels; c++)
        target_img[(target_img_width * target_img_height) * c +
                   y * target_img_width + x] =
            source_img[yy * source_img_width + xx];
  }
}