/*
  OpenCL Kernel capable of wraping an image
  to an equirectangular projection using a Thin
  Plate Spline transformation that was already
  solved on the CPU.

  This has a huge performance boost over doing
  it purely on CPU, unless you have a potato
  as a GPU of course!

  The TPS code is nearly a 1:1 port of Vizz's
  in C++. Only 2D is supported as realistically
  for any usecase worth doing on GPU... It will
  be 2D.

  This is the 64-bits float (double) version,
  which has better accuracy (only required for
  very high resolution usecases).
*/

#define DEG_TO_RAD (3.14159265359 / 180.0)
#define RAD_TO_DEG (180.0 / 3.14159265359)

inline double lon_shift(double lon, double shift) {
  lon += shift;
  if (lon > 180)
    lon -= 360;
  if (lon < -180)
    lon += 360;
  return lon;
}

inline void shift_latlon_by_lat(double *lat, double *lon, double shift) {
  if (shift == 0)
    return;

  double x = cos(*lat * DEG_TO_RAD) * cos(*lon * DEG_TO_RAD);
  double y = cos(*lat * DEG_TO_RAD) * sin(*lon * DEG_TO_RAD);
  double z = sin(*lat * DEG_TO_RAD);

  double theta = shift * DEG_TO_RAD;

  double x2 = x * cos(theta) + z * sin(theta);
  double y2 = y;
  double z2 = z * cos(theta) - x * sin(theta);

  *lon = atan2(y2, x2) * RAD_TO_DEG;
  double hyp = sqrt(x2 * x2 + y2 * y2);
  *lat = atan2(z2, hyp) * RAD_TO_DEG;
}

inline double SQ(const double x) { return x * x; }

inline void VizGeorefSpline2DBase_func4(double *res, const double *pxy,
                                        global const double *xr,
                                        global const double *yr) {
  double dist0 = SQ(xr[0] - pxy[0]) + SQ(yr[0] - pxy[1]);
  res[0] = dist0 != 0.0 ? dist0 * log(dist0) : 0.0;
  double dist1 = SQ(xr[1] - pxy[0]) + SQ(yr[1] - pxy[1]);
  res[1] = dist1 != 0.0 ? dist1 * log(dist1) : 0.0;
  double dist2 = SQ(xr[2] - pxy[0]) + SQ(yr[2] - pxy[1]);
  res[2] = dist2 != 0.0 ? dist2 * log(dist2) : 0.0;
  double dist3 = SQ(xr[3] - pxy[0]) + SQ(yr[3] - pxy[1]);
  res[3] = dist3 != 0.0 ? dist3 * log(dist3) : 0.0;
}

inline double VizGeorefSpline2DBase_func(const double x1, const double y1,
                                         const double x2, const double y2) {
  const double dist = SQ(x2 - x1) + SQ(y2 - y1);
  return dist != 0.0 ? dist * log(dist) : 0.0;
}

inline ushort get_pixel_bilinear(global const ushort *img, const int width,
                                 const int height, const int channels, int cc, double rx,
                                 double ry) {
  int x = (int)rx;
  int y = (int)ry;

  double x_diff = rx - x;
  double y_diff = ry - y;

  size_t index = (y * width + x);
  size_t max_index = width * height;

  int a = 0, b = 0, c = 0, d = 0;
  double a_a = 1.0, b_a = 1.0, c_a = 1.0, d_a = 1.0;

  a = img[cc * width * height + index];
  if (channels == 4 && cc != 3)
    a_a = img[3 * width * height + index] / 65535.0;

  if (index + 1 < max_index) {
    b = img[cc * width * height + index + 1];
    if (channels == 4 && cc != 3) {
      b_a = img[3 * width * height + index + 1] / 65535.0;
      b = (double)b * b_a;
    }
  }
  else
    return a;

  if (index + width < max_index) {
    c = img[cc * width * height + index + width];
    if (channels == 4 && cc != 3) {
      c_a = img[3 * width * height + index + width] / 65535.0;
      c = (double)c * c_a;
    }
  }
  else
    return a;

  if (index + width + 1 < max_index) {
    d = img[cc * width * height + index + width + 1];
    if (channels == 4 && cc != 3) {
      d_a = img[3 * width * height + index + width + 1] / 65535.0;
      d = (double)d * d_a;
    }
  }
  else
    return a;

  if (x == width - 1)
    return a;
  if (y == height - 1)
    return a;

  a = (double)a * a_a;
  int val = (int)((double)a * (1.0 - x_diff) * (1.0 - y_diff) +
                  (double)b * (x_diff) * (1.0 - y_diff) +
                  (double)c * (y_diff) * (1.0 - x_diff) +
                  (double)d * (x_diff * y_diff));
  if (channels == 4 && cc != 3) {
    val = (int)((double)val / (a_a * (1 - x_diff) * (1 - y_diff) +
                b_a * (x_diff) * (1 - y_diff) +
                c_a * (y_diff) * (1 - x_diff) +
                d_a * (x_diff * y_diff)));
  }

  if (val > 65535)
    val = 65535;
  return val;
}

void kernel warp_image_thin_plate_spline(
    global ushort *map_image, global ushort *img, global int *tps_no_points,
    global double *tps_x, global double *tps_y, global double *tps_coef_1,
    global double *tps_coef_2, global double *tps_xmean,
    global double *tps_ymean, global int *img_settings) {

  int id = get_global_id(0);
  int nthreads = get_global_size(0);

  int map_img_width = img_settings[0];
  int map_img_height = img_settings[1];
  int crop_width = img_settings[9] - img_settings[8];
  int crop_height = img_settings[7] - img_settings[6];
  int img_width = img_settings[2];
  int img_height = img_settings[3];
  int source_channels = img_settings[4];
  int target_channels = img_settings[5];
  int lon_shiftv = img_settings[10];
  int lat_shiftv = img_settings[11];

  size_t n = crop_width * crop_height;

  size_t ratio = (n / nthreads); // number of elements for each thread
  size_t start = ratio * id;
  size_t stop = ratio * (id + 1);

  // Init TPS

  double xx, yy;

  double vars[2];
  global const double *coef[2] = {tps_coef_1, tps_coef_2};
  const int _nof_points = *tps_no_points;
  const int _nof_vars = 2;
  const double x_mean = *tps_xmean;
  const double y_mean = *tps_ymean;

  for (size_t xy_ptr = start; xy_ptr < stop; xy_ptr++) {
    int x = (xy_ptr % crop_width);
    int y = (xy_ptr / crop_width);

    // Scale to the map
    double lat =
        -((double)(y + img_settings[6]) / (double)map_img_height) * 180 + 90;
    double lon =
        ((double)(x + img_settings[8]) / (double)map_img_width) * 360 - 180;

    // Perform TPS
    {
      shift_latlon_by_lat(&lat, &lon, (double)lat_shiftv);
      const double Pxy[2] = {lon_shift(lon, (double)lon_shiftv) - x_mean,
                             lat - y_mean};
      for (int v = 0; v < _nof_vars; v++)
        vars[v] = coef[v][0] + coef[v][1] * Pxy[0] + coef[v][2] * Pxy[1];

      int r = 0; // Used after for.
      for (; r < (_nof_points & (~3)); r += 4) {
        double dfTmp[4] = {};
        VizGeorefSpline2DBase_func4(dfTmp, Pxy, &tps_x[r], &tps_y[r]);
        for (int v = 0; v < _nof_vars; v++)
          vars[v] += coef[v][r + 3] * dfTmp[0] + coef[v][r + 3 + 1] * dfTmp[1] +
                     coef[v][r + 3 + 2] * dfTmp[2] +
                     coef[v][r + 3 + 3] * dfTmp[3];
      }
      for (; r < _nof_points; r++) {
        const double tmp =
            VizGeorefSpline2DBase_func(Pxy[0], Pxy[1], tps_x[r], tps_y[r]);
        for (int v = 0; v < _nof_vars; v++)
          vars[v] += coef[v][r + 3] * tmp;
      }

      xx = vars[0];
      yy = vars[1];
    }

    if (xx < 0 || yy < 0)
      continue;

    if ((int)xx > img_width - 1 || (int)yy > img_height - 1)
      continue;

    if (target_channels == 4) {
      if (source_channels == 1)
        for (int c = 0; c < 3; c++)
          map_image[(crop_width * crop_height) * c + y * crop_width + x] =
              get_pixel_bilinear(img, img_width, img_height, source_channels, 0, xx,
                                 yy); // img[(int)yy * img_width + (int)xx];
      else if (source_channels == 3 || source_channels == 4)
        for (int c = 0; c < 3; c++)
          map_image[(crop_width * crop_height) * c + y * crop_width + x] =
              get_pixel_bilinear(img, img_width, img_height, source_channels, c, xx,
                                 yy); // img[(img_width * img_height) * c +
                                      // (int)yy * img_width + (int)xx];
      if (source_channels == 4)
        map_image[(crop_width * crop_height) * 3 + y * crop_width + x] =
            get_pixel_bilinear(img, img_width, img_height, source_channels, 3, xx,
                               yy); // img[(img_width * img_height) * 3 +
                                    // (int)yy * img_width + (int)xx];
      else
        map_image[(crop_width * crop_height) * 3 + y * crop_width + x] = 65535;
    } else {
      for (int c = 0; c < source_channels; c++)
        map_image[(crop_width * crop_height) * c + y * crop_width + x] =
            get_pixel_bilinear(img, img_width, img_height, source_channels, c, xx,
                               yy); // img[(img_width * img_height) * c +
                                    // (int)yy * img_width + (int)xx];
    }
  }
}