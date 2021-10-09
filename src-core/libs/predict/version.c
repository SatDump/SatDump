#include "predict.h"

int predict_version_major()
{
  return PREDICT_VERSION_MAJOR;
}

int predict_version_minor()
{
  return PREDICT_VERSION_MINOR;
}

int predict_version_patch()
{
  return PREDICT_VERSION_PATCH;
}

int predict_version()
{
  return PREDICT_VERSION;
}

char *predict_version_string()
{
  return PREDICT_VERSION_STRING;
}
