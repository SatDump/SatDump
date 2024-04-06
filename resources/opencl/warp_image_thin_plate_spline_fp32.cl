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

  This is the 32-bits float (float) version,
  which allows using the more common (for consumer
  cards) FP32 cores.
*/

#define DEG_TO_RAD (3.14159265359f / 180.0f)
#define RAD_TO_DEG (180.0f / 3.14159265359f)

inline float lon_shift(float lon, float shift) {
  lon += shift;
  if (lon > 180)
    lon -= 360;
  if (lon < -180)
    lon += 360;
  return lon;
}

inline void shift_latlon_by_lat(float *lat, float *lon, float shift) {
  if (shift == 0)
    return;

  float x = cos(*lat * DEG_TO_RAD) * cos(*lon * DEG_TO_RAD);
  float y = cos(*lat * DEG_TO_RAD) * sin(*lon * DEG_TO_RAD);
  float z = sin(*lat * DEG_TO_RAD);

  float theta = shift * DEG_TO_RAD;

  float x2 = x * cos(theta) + z * sin(theta);
  float y2 = y;
  float z2 = z * cos(theta) - x * sin(theta);

  *lon = atan2(y2, x2) * RAD_TO_DEG;
  float hyp = sqrt(x2 * x2 + y2 * y2);
  *lat = atan2(z2, hyp) * RAD_TO_DEG;
}

inline float SQ(const float x) { return x * x; }

inline void VizGeorefSpline2DBase_func4(float *res, const float *pxy,
                                        global const float *xr,
                                        global const float *yr) {
  float dist0 = SQ(xr[0] - pxy[0]) + SQ(yr[0] - pxy[1]);
  res[0] = dist0 != 0.0f ? dist0 * log(dist0) : 0.0f;
  float dist1 = SQ(xr[1] - pxy[0]) + SQ(yr[1] - pxy[1]);
  res[1] = dist1 != 0.0f ? dist1 * log(dist1) : 0.0f;
  float dist2 = SQ(xr[2] - pxy[0]) + SQ(yr[2] - pxy[1]);
  res[2] = dist2 != 0.0f ? dist2 * log(dist2) : 0.0f;
  float dist3 = SQ(xr[3] - pxy[0]) + SQ(yr[3] - pxy[1]);
  res[3] = dist3 != 0.0f ? dist3 * log(dist3) : 0.0f;
}

inline float VizGeorefSpline2DBase_func(const float x1, const float y1,
                                        const float x2, const float y2) {
  const float dist = SQ(x2 - x1) + SQ(y2 - y1);
  return dist != 0.0f ? dist * log(dist) : 0.0f;
}

inline ushort get_pixel_bilinear(global const ushort *img, const int width,
                                 const int height, const int channels, int cc,
                                 float rx, float ry) {
  int x = (int)rx;
  int y = (int)ry;

  float x_diff = rx - x;
  float y_diff = ry - y;

  size_t index = (y * width + x);
  size_t max_index = width * height;

  int a = 0, b = 0, c = 0, d = 0;
  float a_a = 1.0f, b_a = 1.0f, c_a = 1.0f, d_a = 1.0f;

  a = img[cc * width * height + index];
  if (channels == 4 && cc != 3)
    a_a = img[3 * width * height + index] / 65535.0f;

  if (index + 1 < max_index) {
    b = img[cc * width * height + index + 1];
    if (channels == 4 && cc != 3) {
      b_a = img[3 * width * height + index + 1] / 65535.0f;
      b = (float)b * b_a;
    }
  }
  else
    return a;

  if (index + width < max_index) {
    c = img[cc * width * height + index + width];
    if (channels == 4 && cc != 3) {
      c_a = img[3 * width * height + index + width] / 65535.0f;
      c = (float)c * c_a;
    }
  }
  else
    return a;

  if (index + width + 1 < max_index) {
    d = img[cc * width * height + index + width + 1];
    if (channels == 4 && cc != 3) {
      d_a = img[3 * width * height + index + width + 1] / 65535.0f;
      d = (float)d * d_a;
    }
  }
  else
    return a;

  if (x == width - 1)
    return a;
  if (y == height - 1)
    return a;

  a = (float)a * a_a;
  int val = (int)((float)a * (1.0 - x_diff) * (1.0 - y_diff) +
                  (float)b * (x_diff) * (1.0 - y_diff) +
                  (float)c * (y_diff) * (1.0 - x_diff) +
                  (float)d * (x_diff * y_diff));
  if (channels == 4 && cc != 3) {
    val = (int)((float)val / (a_a * (1 - x_diff) * (1 - y_diff) +
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
    global float *tps_x, global float *tps_y, global float *tps_coef_1,
    global float *tps_coef_2, global float *tps_xmean, global float *tps_ymean,
    global int *img_settings) {

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

  float xx, yy;

  float vars[2];
  global const float *coef[2] = {tps_coef_1, tps_coef_2};
  const int _nof_points = *tps_no_points;
  const int _nof_vars = 2;
  const float x_mean = *tps_xmean;
  const float y_mean = *tps_ymean;

  for (size_t xy_ptr = start; xy_ptr < stop; xy_ptr++) {
    int x = (xy_ptr % crop_width);
    int y = (xy_ptr / crop_width);

    // Scale to the map
    float lat =
        -((float)(y + img_settings[6]) / (float)map_img_height) * 180 + 90;
    float lon =
        ((float)(x + img_settings[8]) / (float)map_img_width) * 360 - 180;

    // Perform TPS
    {
      shift_latlon_by_lat(&lat, &lon, (float)lat_shiftv);
      const float Pxy[2] = {lon_shift(lon, (float)lon_shiftv) - x_mean,
                            lat - y_mean};
      for (int v = 0; v < _nof_vars; v++)
        vars[v] = coef[v][0] + coef[v][1] * Pxy[0] + coef[v][2] * Pxy[1];

      int r = 0; // Used after for.
      for (; r < (_nof_points & (~3)); r += 4) {
        float dfTmp[4] = {};
        VizGeorefSpline2DBase_func4(dfTmp, Pxy, &tps_x[r], &tps_y[r]);
        for (int v = 0; v < _nof_vars; v++)
          vars[v] += coef[v][r + 3] * dfTmp[0] + coef[v][r + 3 + 1] * dfTmp[1] +
                     coef[v][r + 3 + 2] * dfTmp[2] +
                     coef[v][r + 3 + 3] * dfTmp[3];
      }
      for (; r < _nof_points; r++) {
        const float tmp =
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