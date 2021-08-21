
#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#define MINIZ_STATIC_DEFINE

#ifdef MINIZ_STATIC_DEFINE
#  define MINIZ_EXPORT
#  define MINIZ_NO_EXPORT
#else
#  ifndef MINIZ_EXPORT
#    ifdef miniz_EXPORTS
        /* We are building this library */
#      define MINIZ_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define MINIZ_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef MINIZ_NO_EXPORT
#    define MINIZ_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef MINIZ_DEPRECATED
#  define MINIZ_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef MINIZ_DEPRECATED_EXPORT
#  define MINIZ_DEPRECATED_EXPORT MINIZ_EXPORT MINIZ_DEPRECATED
#endif

#ifndef MINIZ_DEPRECATED_NO_EXPORT
#  define MINIZ_DEPRECATED_NO_EXPORT MINIZ_NO_EXPORT MINIZ_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef MINIZ_NO_DEPRECATED
#    define MINIZ_NO_DEPRECATED
#  endif
#endif

#endif /* MINIZ_EXPORT_H */
