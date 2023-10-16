/*
  OpenCL Kernel capable of reprojecting
  an equirectangular projection to a
  Azimuthal Equidistant one.

  This is faster on the GPU :-)
*/

struct azeq_cfg {
  int image_height;
  int image_width;

  float center_lat;
  float center_lon;

  float center_phi;
  float center_lam;
};

void azeq_inverse(struct azeq_cfg *cfg, int x, int y, float *lon, float *lat) {
  // x = image_width - x;
  y = cfg->image_height - y;
  float px, py, c;

  px = (2.0f * 3.14159265358979323846f * (float)x) / (float)cfg->image_width -
       3.14159265358979323846f;
  py = (2.0f * 3.14159265358979323846f * (float)y) / (float)cfg->image_height -
       3.14159265358979323846f;
  c = sqrt(pow(px, 2) + pow(py, 2));

  if (c > 3.14159265358979323846f) {
    *lat = -1;
    *lon = -1;
    return;
  }
  *lat = 57.29578f * asin(cos(c) * sin(cfg->center_phi) +
                          py * sin(c) * cos(cfg->center_phi) / c);

  if (cfg->center_lat == 90) {
    *lon = (cfg->center_lam + atan2(-1 * px, py)) * 57.29578f;
  } else if (cfg->center_lat == -90) {
    *lon = (cfg->center_lam + atan2(px, py)) * 57.29578f;
  } else {
    *lon = (cfg->center_lam +
            atan2(px * sin(c), (c * cos(cfg->center_phi) * cos(c) -
                                py * sin(cfg->center_phi) * sin(c)))) *
           57.29578f;
  }

  if (*lon < -180)
    *lon += 360;
  if (*lon > 180)
    *lon -= 360;
}

void kernel reproj_image_equ_to_azeq(global ushort *source_img,
                                     global ushort *target_img,
                                     global int *img_sizes,
                                     global float *img_equ_settings,
                                     global float *img_azeq_settings) {

  int id = get_global_id(0);
  int nthreads = get_global_size(0);

  // Image sizes
  int source_img_width = img_sizes[0];
  int source_img_height = img_sizes[1];
  int target_img_width = img_sizes[2];
  int target_img_height = img_sizes[3];
  int source_channels = img_sizes[4];
  int target_channels = img_sizes[5];

  // Equirectangular settings
  float top_left_lat = img_equ_settings[0];
  float top_left_lon = img_equ_settings[1];
  float bottom_right_lat = img_equ_settings[2];
  float bottom_right_lon = img_equ_settings[3];
  float covered_lat = fabs(top_left_lat - bottom_right_lat);
  float covered_lon = fabs(top_left_lon - bottom_right_lon);
  float offset_lat = fabs(top_left_lat - 90);
  float offset_lon = fabs(top_left_lon + 180);

  // azeq settings
  struct azeq_cfg azeq_cfg;
  azeq_cfg.image_height = img_azeq_settings[0];
  azeq_cfg.image_width = img_azeq_settings[1];
  azeq_cfg.center_lat = img_azeq_settings[2];
  azeq_cfg.center_lon = img_azeq_settings[3];
  azeq_cfg.center_phi = img_azeq_settings[4];
  azeq_cfg.center_lam = img_azeq_settings[5];

  size_t n = target_img_width * target_img_height;

  size_t ratio = (n / nthreads); // number of elements for each thread
  size_t start = ratio * id;
  size_t stop = ratio * (id + 1);

  float lon = 0, lat = 0;
  int xx = 0, yy = 0;

  for (size_t xy_ptr = start; xy_ptr < stop; xy_ptr++) {
    int x = (xy_ptr % target_img_width);
    int y = (xy_ptr / target_img_width);

    // float px = (x - (target_img_width / 2));
    // float py = (target_img_height - y) - (target_img_height / 2);

    // px /= target_img_width / 2;
    // py /= target_img_height / 2;

    azeq_inverse(&azeq_cfg, x, y, &lon, &lat);
    if (lon == -1 || lat == -1)
      continue;

    if (lat > top_left_lat || lat < bottom_right_lat || lon < top_left_lon ||
        lon > bottom_right_lon)
      continue;

    lat = 180.0f - (lat + 90.0f);
    lon += 180;

    lat -= offset_lat;
    lon -= offset_lon;

    yy = (lat / covered_lat) * source_img_height;
    xx = (lon / covered_lon) * source_img_width;

    if (yy < 0 || yy >= source_img_height || xx < 0 || xx >= source_img_width)
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