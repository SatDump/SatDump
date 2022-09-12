/*
  OpenCL Kernel capable of reprojecting
  an equirectangular projection to a
  stereographic one.

  This is faster on the GPU :-)
*/

float const E = 2.718281828459f;

float logf(float x) {
  if (x == 0.0f) {
    return -INFINITY;
  }

  float p = 0.0f;
  while (x < 0.5f) {
    x *= E;
    p += 1;
  }

  x -= 1.0f;
  float r = 0.0f, c = -1.0f;
  for (int i = 1; i < 8; ++i) {
    c *= -x;
    r += c / i;
  }

  return r - p;
}

float expf(float x) {
  x = -x;
  float r = 1.0f, c = 1.0f;
  for (int i = 1; i < 8; ++i) {
    c *= x / i;
    r += c;
  }

  return 1.0f / r;
}

float powf(float base, float exponent) {
  if (base == 0.0f)
    return 0.0f;
  return expf(exponent * logf(base));
}

enum stereo_mode_t { S_POLE = 0, N_POLE = 1, OBLIQ = 2, EQUIT = 3 };

struct stereo_cfg {
  float phits;
  float sinX1;
  float cosX1;
  float akm1;
  enum stereo_mode_t mode;

  float e;
  float phi0;
  float a;
  float es;
  float one_es;

  float k0;
  float lam0;

  float lon_0;
};

int stereo_inverse(struct stereo_cfg *cfg, float x, float y, float *lon,
                   float *lat) {
  *lon = *lat = 0.0;
  float phi = 0, lam = 0;

  float cosphi, sinphi, tp = 0.0, phi_l = 0.0, rho, halfe = 0.0, halfpi = 0.0;

  rho = hypot(x, y);

  switch (cfg->mode) {
  case OBLIQ:
  case EQUIT:
    tp = 2. * atan2(rho * cfg->cosX1, cfg->akm1);
    cosphi = cos(tp);
    sinphi = sin(tp);
    if (rho == 0.0)
      phi_l = asin(cosphi * cfg->sinX1);
    else
      phi_l = asin(cosphi * cfg->sinX1 + (y * sinphi * cfg->cosX1 / rho));

    tp = tan(.5 * (M_PI_2 + phi_l));
    x *= sinphi;
    y = rho * cfg->cosX1 * cosphi - y * cfg->sinX1 * sinphi;
    halfpi = M_PI_2;
    halfe = .5 * cfg->e;
    break;
  case N_POLE:
    y = -y;
    /*-fallthrough*/
  case S_POLE:
    tp = -rho / cfg->akm1;
    phi_l = M_PI_2 - 2. * atan(tp);
    halfpi = -M_PI_2;
    halfe = -.5 * cfg->e;
    break;
  }

  for (int i = 8; i > 0; --i) {
    sinphi = cfg->e * sin(phi_l);
    phi = 2. * atan(tp * powf((1. + sinphi) / (1. - sinphi), halfe)) - halfpi;
    if (fabs(phi_l - phi) < 1.e-10) {
      if (cfg->mode == S_POLE)
        phi = -phi;
      lam = (x == 0. && y == 0.) ? 0. : atan2(x, y);

      // To degs
      *lat = phi * 57.29578;
      *lon = lam * 57.29578;

      // Shift longitudes back to reference 0
      *lon += cfg->lon_0;
      if (*lon < -180)
        *lon = *lon + 360;
      if (*lon > 180)
        *lon = *lon - 360;

      return 0;
    }
    phi_l = phi;
  }

  return 1;
}

void kernel reproj_image_equ_to_stereo(global ushort *source_img,
                                       global ushort *target_img,
                                       global int *img_sizes,
                                       global float *img_equ_settings,
                                       global float *img_stereo_settings) {

  int id = get_global_id(0);
  int nthreads = get_global_size(0);

  // Image sizes
  int source_img_width = img_sizes[0];
  int source_img_height = img_sizes[1];
  int target_img_width = img_sizes[2];
  int target_img_height = img_sizes[3];
  int imgs_channels = img_sizes[4];

  // Equirectangular settings
  float top_left_lat = img_equ_settings[0];
  float top_left_lon = img_equ_settings[1];
  float bottom_right_lat = img_equ_settings[2];
  float bottom_right_lon = img_equ_settings[3];
  float covered_lat = fabs(top_left_lat - bottom_right_lat);
  float covered_lon = fabs(top_left_lon - bottom_right_lon);
  float offset_lat = fabs(top_left_lat - 90);
  float offset_lon = fabs(top_left_lon + 180);

  // Stereo settings
  struct stereo_cfg stereo_cfg;
  stereo_cfg.phits = img_stereo_settings[0];
  stereo_cfg.sinX1 = img_stereo_settings[1];
  stereo_cfg.cosX1 = img_stereo_settings[2];
  stereo_cfg.akm1 = img_stereo_settings[3];
  stereo_cfg.mode = img_stereo_settings[4];
  stereo_cfg.e = img_stereo_settings[5];
  stereo_cfg.phi0 = img_stereo_settings[6];
  stereo_cfg.a = img_stereo_settings[7];
  stereo_cfg.es = img_stereo_settings[8];
  stereo_cfg.one_es = img_stereo_settings[9];
  stereo_cfg.k0 = img_stereo_settings[10];
  stereo_cfg.lam0 = img_stereo_settings[11];
  stereo_cfg.lon_0 = img_stereo_settings[12];
  float stereo_scale = img_stereo_settings[13];

  size_t n = target_img_width * target_img_height;

  size_t ratio = (n / nthreads); // number of elements for each thread
  size_t start = ratio * id;
  size_t stop = ratio * (id + 1);

  float lon = 0, lat = 0;
  int xx = 0, yy = 0;

  for (size_t xy_ptr = start; xy_ptr < stop; xy_ptr++) {
    int x = (xy_ptr % target_img_width);
    int y = (xy_ptr / target_img_width);

    float px = (x - (target_img_width / 2));
    float py = (target_img_height - y) - (target_img_height / 2);

    px /= target_img_width / stereo_scale;
    py /= target_img_height / stereo_scale;

    stereo_inverse(&stereo_cfg, px, py, &lon, &lat);
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

    for (int c = 0; c < imgs_channels; c++)
      target_img[(target_img_width * target_img_height) * c +
                 y * target_img_width + x] =
          source_img[(source_img_width * source_img_height) * c +
                     (int)yy * source_img_width + (int)xx];
  }
}