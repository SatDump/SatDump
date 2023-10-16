/*
  OpenCL Kernel capable of reprojecting
  an equirectangular projection to a
  TPERS (Titled Perspective) one.

  This is faster on the GPU :-)
*/

enum tpers_mode_t { N_POLE = 0, S_POLE = 1, EQUIT = 2, OBLIQ = 3 };

struct tpers_cfg {
  float height;
  float sinph0;
  float cosph0;
  float p;
  float rp;
  float pn1;
  float pfact;
  float h;
  float cg;
  float sg;
  float sw;
  float cw;
  enum tpers_mode_t mode;
  int tilt;

  float phi0;
  float a;
  float es;

  float lon_0;
};

#define EPS10 1.e-10f

int tpers_inverse(struct tpers_cfg *cfg, float x, float y, float *lon,
                  float *lat) {
  *lon = *lat = 0.0f;

  float phi = 0, lam = 0;
  float rh;

  if (cfg->tilt) {
    float bm, bq, yt;

    yt = 1. / (cfg->pn1 - y * cfg->sw);
    bm = cfg->pn1 * x * yt;
    bq = cfg->pn1 * y * cfg->cw * yt;
    x = bm * cfg->cg + bq * cfg->sg;
    y = bq * cfg->cg - bm * cfg->sg;
  }

  rh = hypot(x, y);

  if (fabs(rh) <= EPS10) {
    lam = 0.0f;
    phi = cfg->phi0;
  } else {
    float cosz, sinz;
    sinz = 1.0f - rh * rh * cfg->pfact;

    if (sinz < 0.0f) {
      // Illegal
      *lon = *lat = 2e10f; // Trigger error
      return 1;
    }

    sinz = (cfg->p - sqrt(sinz)) / (cfg->pn1 / rh + rh / cfg->pn1);
    cosz = sqrt(1.0f - sinz * sinz);

    switch (cfg->mode) {
    case OBLIQ:
      phi = asin(cosz * cfg->sinph0 + y * sinz * cfg->cosph0 / rh);
      y = (cosz - cfg->sinph0 * sin(phi)) * rh;
      x *= sinz * cfg->cosph0;
      break;
    case EQUIT:
      phi = asin(y * sinz / rh);
      y = cosz * rh;
      x *= sinz;
      break;
    case N_POLE:
      phi = asin(cosz);
      y = -y;
      break;
    case S_POLE:
      phi = -asin(cosz);
      break;
    }

    lam = atan2(x, y);
  }

  // To degs
  *lat = phi * 57.29578f;
  *lon = lam * 57.29578f;

  // Shift longitudes back to reference 0
  *lon += cfg->lon_0;
  if (*lon < -180)
    *lon = *lon + 360;
  if (*lon > 180)
    *lon = *lon - 360;

  return 0;
}

void kernel reproj_image_equ_to_tpers(global ushort *source_img,
                                      global ushort *target_img,
                                      global int *img_sizes,
                                      global float *img_equ_settings,
                                      global float *img_tpers_settings) {

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

  // TPERS settings
  struct tpers_cfg tpers_cfg;
  tpers_cfg.height = img_tpers_settings[0];
  tpers_cfg.sinph0 = img_tpers_settings[1];
  tpers_cfg.cosph0 = img_tpers_settings[2];
  tpers_cfg.p = img_tpers_settings[3];
  tpers_cfg.rp = img_tpers_settings[4];
  tpers_cfg.pn1 = img_tpers_settings[5];
  tpers_cfg.pfact = img_tpers_settings[6];
  tpers_cfg.h = img_tpers_settings[7];
  tpers_cfg.cg = img_tpers_settings[8];
  tpers_cfg.sg = img_tpers_settings[9];
  tpers_cfg.sw = img_tpers_settings[10];
  tpers_cfg.cw = img_tpers_settings[11];
  tpers_cfg.mode = img_tpers_settings[12];
  tpers_cfg.tilt = img_tpers_settings[13];
  tpers_cfg.phi0 = img_tpers_settings[14];
  tpers_cfg.a = img_tpers_settings[15];
  tpers_cfg.es = img_tpers_settings[16];
  tpers_cfg.lon_0 = img_tpers_settings[17];

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

    px /= target_img_width / 2;
    py /= target_img_height / 2;

    tpers_inverse(&tpers_cfg, px, py, &lon, &lat);
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