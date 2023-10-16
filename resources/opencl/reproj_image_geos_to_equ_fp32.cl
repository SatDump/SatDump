/*
  OpenCL Kernel capable of reprojecting
  a GEOS projection to an equirectangular
  one.

  This is faster on the GPU :-)
*/

struct geos_cfg {
  float h;
  float radius_p;
  float radius_p2;
  float radius_p_inv2;
  float radius_g;
  float radius_g_1;
  float C;
  int flip_axis;

  float phi0;
  float a;
  float es;
  float one_es;

  float lon_0;
};

int geos_forward(struct geos_cfg *cfg, float lon, float lat, float *x,
                 float *y) {
  *x = *y = 0; // Safety

  // Shift longitudes to use the sat's as a reference
  lon -= cfg->lon_0;
  if (lon < -180)
    lon = lon + 360;
  if (lon > 180)
    lon = lon - 360;

  // To radians
  float phi = lat * 0.01745329f, lam = lon * 0.01745329f;

  float r, Vx, Vy, Vz, tmp;

  // Calculation of geocentric latitude.
  phi = atan(cfg->radius_p2 * tan(phi));

  // Calculation of the three components of the vector from satellite to
  // position on earth surface (lon,lat).
  r = (cfg->radius_p) / hypot(cfg->radius_p * cos(phi), sin(phi));
  Vx = r * cos(lam) * cos(phi);
  Vy = r * sin(lam) * cos(phi);
  Vz = r * sin(phi);

  // Check visibility.
  if (((cfg->radius_g - Vx) * Vx - Vy * Vy - Vz * Vz * cfg->radius_p_inv2) <
      0.) {
    *x = *y = 2e10f; // Trigger error
    return 1;
  }

  // Calculation based on view angles from satellite.
  tmp = cfg->radius_g - Vx;

  if (cfg->flip_axis) {
    *x = cfg->radius_g_1 * atan(Vy / hypot(Vz, tmp));
    *y = cfg->radius_g_1 * atan(Vz / tmp);
  } else {
    *x = cfg->radius_g_1 * atan(Vy / tmp);
    *y = cfg->radius_g_1 * atan(Vz / hypot(Vy, tmp));
  }

  return 0;
}

struct geo_proj_cfg {
  struct geos_cfg *geos_cfg;
  float height, width;
  float hscale;
  float vscale;
  float x_offset;
  float y_offset;
};

int geo_proj_forward(struct geo_proj_cfg *cfg, float lon, float lat, int *img_x,
                     int *img_y) {
  float x, y;
  float image_x, image_y;
  if (geos_forward(cfg->geos_cfg, lon, lat, &x, &y)) {
    // Error / out of the image
    *img_x = -1;
    *img_y = -1;
    return 1;
  }

  y -= cfg->y_offset;
  x -= cfg->x_offset;

  image_x = x * cfg->hscale * (cfg->width / 2.0f);
  image_y = y * cfg->vscale * (cfg->height / 2.0f);

  image_x += cfg->width / 2.0f;
  image_y += cfg->height / 2.0f;

  *img_x = image_x;
  *img_y = (cfg->height - 1) - image_y;

  if (*img_x >= cfg->width || *img_y >= cfg->height)
    return 1;
  if (*img_x < 0 || *img_y < 0)
    return 1;

  return 0;
}

void kernel reproj_image_geos_to_equ(global ushort *source_img,
                                     global ushort *target_img,
                                     global int *img_sizes,
                                     global float *img_geos_settings,
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

  // GEOS settings
  struct geos_cfg geos_cfg;
  geos_cfg.h = img_geos_settings[0];
  geos_cfg.radius_p = img_geos_settings[1];
  geos_cfg.radius_p2 = img_geos_settings[2];
  geos_cfg.radius_p_inv2 = img_geos_settings[3];
  geos_cfg.radius_g = img_geos_settings[4];
  geos_cfg.radius_g_1 = img_geos_settings[5];
  geos_cfg.C = img_geos_settings[6];
  geos_cfg.flip_axis = img_geos_settings[7];
  geos_cfg.phi0 = img_geos_settings[8];
  geos_cfg.a = img_geos_settings[9];
  geos_cfg.es = img_geos_settings[10];
  geos_cfg.one_es = img_geos_settings[11];
  geos_cfg.lon_0 = img_geos_settings[12];
  struct geo_proj_cfg geo_proj_cfg;
  geo_proj_cfg.geos_cfg = &geos_cfg;
  geo_proj_cfg.width = source_img_width;
  geo_proj_cfg.height = source_img_height;
  geo_proj_cfg.hscale = img_geos_settings[13];
  geo_proj_cfg.vscale = img_geos_settings[14];
  geo_proj_cfg.x_offset = img_geos_settings[15];
  geo_proj_cfg.y_offset = img_geos_settings[16];

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

    // Lat/Lon => GEOS
    geo_proj_forward(&geo_proj_cfg, lon, lat, &xx, &yy);
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