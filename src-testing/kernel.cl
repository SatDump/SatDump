inline double SQ(const double x) { return x * x; }

inline void VizGeorefSpline2DBase_func4(double *res, const double *pxy,
                                        const double *xr, const double *yr) {
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

void kernel warp_image_thin_plate_spline(
    global ushort *map_image, global ushort *img, global int *tps_no_points,
    global double *tps_x, global double *tps_y, global double *tps_coef_1,
    global double *tps_coef_2, global double *tps_xmean,
    global double *tps_ymean, global int *img_settings) {

  int id = get_global_id(0);
  int nthreads = get_global_size(0);

  int map_img_width = img_settings[0];
  int map_img_height = img_settings[1];
  int crop_width = img_settings[8] - img_settings[7];
  int crop_height = img_settings[6] - img_settings[5];
  int img_width = img_settings[2];
  int img_height = img_settings[3];
  int img_channels = img_settings[4];

  size_t n = crop_width * crop_height;

  size_t ratio = (n / nthreads); // number of elements for each thread
  size_t start = ratio * id;
  size_t stop = ratio * (id + 1);

  // Init TPS

  double xx, yy;

  double vars[2];
  double *coef[2] = {tps_coef_1, tps_coef_2};
  int _nof_points = *tps_no_points;
  int _nof_vars = 2;
  double x_mean = *tps_xmean;
  double y_mean = *tps_ymean;

  for (size_t xy_ptr = start; xy_ptr < stop; xy_ptr++) {
    int x = (xy_ptr % crop_width);
    int y = (xy_ptr / crop_width);

    // Scale to the map
    double lat =
        -((double)(y + img_settings[5]) / (double)map_img_height) * 180 + 90;
    double lon =
        ((double)(x + img_settings[7]) / (double)map_img_width) * 360 - 180;

    // Perform TPS
    {
      const double Pxy[2] = {lon - x_mean, lat - y_mean};
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

    for (int c = 0; c < img_channels; c++)
      map_image[(crop_width * crop_height) * c + y * crop_width + x] =
          img[(img_width * img_height) * c + (int)yy * img_width + (int)xx];
  }
}