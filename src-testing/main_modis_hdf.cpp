/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include <netcdf.h>
#include <mfhdf.h>
#include <utility>

#include "common/utils.h"

#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"

char *sanitize_string(char *str, bool sanitize)
{
    char *new_str = NULL;

    if (!str)
        return NULL;

    new_str = strdup(str);
    if (NULL == new_str)
    {
        printf("Out of memory!\n");
        return NULL;
    }

    if (sanitize)
    {
        char *ptr = new_str;

        /* Convert all non-alpha, non-numeric, non-hyphen, non-underscore
         * characters to underscores
         */
        for (; *ptr; ptr++)
            if (!isalnum(*ptr) && *ptr != '_' && *ptr != '-')
                *ptr = '_';
    }

    return new_str;
}

std::string type_name(nc_type type)
{
    switch (type)
    {
    case NC_BYTE:
        return "byte";
    case NC_CHAR:
        return "char";
    case NC_SHORT:
        return "short";
    case NC_LONG:
        return "long";
    case NC_FLOAT:
        return "float";
    case NC_DOUBLE:
        return "double";
    default:
        // error("type_name: bad type %d", type);
        return "bogus";
    }
}

static char *formats[] = {
    "%d",    /* bytes, shorts */
    "%s",    /* char arrays as strings */
    "%ld",   /* longs */
    "%.7g ", /* floats */
    "%.15g"  /* doubles */
};

static int upcorner(long *dims, int ndims, long *odom, long *add)
{
    int id;
    int ret = 1;

    for (id = ndims - 1; id > 0; id--)
    {
        odom[id] += add[id];
        if (odom[id] >= dims[id])
        {
            odom[id - 1]++;
            odom[id] -= dims[id];
        }
    }
    odom[0] += add[0];
    if (odom[0] >= dims[0])
        ret = 0;
    return ret;
}

char *get_fmt(int ncid, int varid, nc_type type)
{
    //  char *c_format_att = has_c_format_att(ncid, varid);

    /* If C_format attribute exists, return it */
    //  if (c_format_att)
    //     return c_format_att;

    /* Otherwise return sensible default. */
    switch (type)
    {
    case NC_BYTE:
        return formats[0];
    case NC_CHAR:
        return formats[1];
    case NC_SHORT:
        return formats[0];
    case NC_LONG:
        return formats[2];
    case NC_FLOAT:
        return formats[3];
    case NC_DOUBLE:
        return formats[4];
    default:
        printf("pr_vals: bad type\n");
        return NULL;
    }
}

template <typename T>
std::string dumpArray(T *vals_pr, nlohmann::json &json_output, char *fmt, int depth, long dim_sizes[], int &pos, int curr_dim = 0)
{
    std::string final_decl;
    final_decl += "{";
    for (int i = 0; i < dim_sizes[curr_dim]; i++)
    {
        if (depth > 1)
            final_decl += dumpArray(vals_pr, json_output[i], fmt, depth - 1, dim_sizes, pos, curr_dim + 1);
        else
        {
            final_decl += svformat(fmt, vals_pr[pos]); // std::to_string(*(vals_pr++));
            json_output[i] = vals_pr[pos++];
        }
        if (i + 1 < dim_sizes[curr_dim])
            final_decl += ", ";
    }
    // logger->critical("POS %d - %d", depth, pos);
    final_decl += "}";
    return final_decl;
}

int main(int /*argc*/, char *argv[])
{
    initLogger();
    completeLoggerInit();

    nlohmann::json json_output;

    int ncid = ncopen(argv[1], NC_NOWRITE); /* netCDF id */

    if (ncid == -1)
    {
        printf("ncopen failed on %s\n", argv[1]);
        return 1;
    }

    /*
     * get number of dimensions, number of variables, number of global
     * atts, and dimension id of unlimited dimension, if any
     */
    int ndims = 0;
    int nvars = 0;
    int ngatts = 0;
    int xdimid = 0;
    (void)ncinquire(ncid, &ndims, &nvars, &ngatts, &xdimid);

    std::vector<std::pair<std::string, int>> all_dims;

    std::vector<std::string> vals_json_intrusive;

    if (ndims > 0)
    {
        for (int dimid = 0; dimid < ndims; dimid++)
        {
            char name[1000];
            long dim_size = 0;
            char *fixed_str = NULL;

            if (ncdiminq(ncid, dimid, name, &dim_size) < 0)
                fprintf(stderr, "Error calling ncdiminq on dimid = %d\n", dimid);

            fixed_str = sanitize_string(name, true);
            if (fixed_str == NULL)
            {
                (void)ncclose(ncid);
                return 1;
            }

            printf("#define %s %d\n", fixed_str, dim_size);

            all_dims.push_back({std::string(fixed_str), dim_size});

            free(fixed_str);
        }
    }

    printf("\n");

    printf("struct HDF4File {\n");

    vals_json_intrusive.push_back("HDF4File");

    /* get variable info, with variable attributes */
    for (int varid = 0; varid < nvars; varid++)
    {
        char name[1000];
        nc_type type;
        int ndims = 0;
        int dims[100];
        int natts;
        char *fixed_str = NULL;

        if (ncvarinq(ncid, varid, name, &type, &ndims, dims, &natts) < 0)
            fprintf(stderr, "Error calling ncvarinq on dimid = %d\n", varid);

        fixed_str = sanitize_string(name, true);
        if (fixed_str == NULL)
        {
            (void)ncclose(ncid);
            return 1;
        }

        printf("    %s %s ", type_name(type).c_str(), fixed_str);

        // if (ndims > 0)
        //     printf("[");

        long vdims[100];

        // std::string final_type_str = "    ";

        // for (int id = 0; id < ndims; id++)
        //    final_type_str += "std::array<";

        int total_dim = 1;

        for (int id = 0; id < ndims; id++)
        {
            char *fixed_dim = sanitize_string((char *)all_dims[dims[id]].first.c_str(), true);
            vdims[id] = all_dims[dims[id]].second;

            total_dim *= vdims[id];

            if (fixed_dim == NULL)
            {
                (void)ncclose(ncid);
                // free(fixed_var);
                return 1;
            }

            //  if (id + 1 == ndims)
            //      final_type_str += type_name(type);
            //            final_type_str +=  "std::array< , " + std::string(fixed_dim )

            //  printf("%s%s", fixed_dim, id < ndims - 1 ? "][ " : "]");
            free(fixed_dim);
        }

        printf("[%d]", total_dim);
        vals_json_intrusive.push_back(std::string(fixed_str));

        //  for (int id = ndims - 1; id >= 0; id--)
        //      final_type_str += ", " + std::string(sanitize_string((char *)all_dims[dims[id]].first.c_str(), true)) + ">";

        //   final_type_str += " " + std::string(fixed_str);

        //  printf("%s", final_type_str.c_str());

        std::vector<float> float_vals;
        std::vector<short> short_vals;
        std::vector<long> long_vals;
        std::vector<char> byte_vals;

        {
            long cor[H4_MAX_VAR_DIMS]; /* corner coordinates */
            long edg[H4_MAX_VAR_DIMS]; /* edges of hypercube */
            long add[H4_MAX_VAR_DIMS]; /* "odometer" increment to next "row"  */
#define VALBUFSIZ 8192
            double vals[VALBUFSIZ / sizeof(double)]; /* aligned buffer */
            int gulp = VALBUFSIZ / nctypelen(type);

            int id;
            int ir;
            long nels;
            long ncols;
            long nrows;
            int vrank = ndims;
            // char *fixed_var;
            int ret = 0, err_code = 0;

            /* printf format used to print each value */
            // const char *fmt = get_fmt(ncid, varid, type);

            nels = 1;
            for (id = 0; id < vrank; id++)
            {
                cor[id] = 0;
                edg[id] = 1;
                nels *= vdims[id]; /* total number of values for variable */
            }

            if (vrank <= 1)
            {
                // printf("\n %s = ", fixed_str);
                // set_indent(strlen(fixed_var) + 4);
            }
            else
            {
                // printf("\n %s =\n  ", fixed_str);
                // set_indent(2);
            }

            if (vrank < 1)
            {
                ncols = 1;
            }
            else
            {
                ncols = vdims[vrank - 1]; /* size of "row" along last dimension */
                edg[vrank - 1] = vdims[vrank - 1];
                for (id = 0; id < vrank; id++)
                    add[id] = 0;
                if (vrank > 1)
                    add[vrank - 2] = 1;
            }

            // printf("\nCOLS %d VRANK %d\n", ncols, vrank);

            nrows = nels / ncols; /* number of "rows" */

            for (ir = 0; ir < nrows; ir++)
            {
                /*
                 * rather than just printing a whole row at once (which might exceed
                 * the capacity of MSDOS platforms, for example), we break each row
                 * into smaller chunks, if necessary.
                 */
                long corsav = 0;
                long left = ncols;
                bool lastrow;

                lastrow = (ir == nrows - 1) ? true : false;
                while (left > 0)
                {
                    long toget = left < gulp ? left : gulp;
                    if (vrank > 0)
                        edg[vrank - 1] = toget;

                    /* ncvarget was casted to (void), thus ncdump misinformed users
                       that the reading succeeded even though the data was corrupted and
                       reading in fact failed (HDFFR-1468.)  Now when ncvarget fails,
                       break out of the while loop and set error code to indicate this
                       failure. -BMR, 2015/01/19 */
                    ret = ncvarget(ncid, varid, cor, edg, (void *)vals);
                    if (ret == -1)
                    {
                        //  err_code = ERR_READFAIL; /* to be returned to caller to
                        //                              indicate that ncvarget fails */
                        break;
                    }

                    // if (fsp->full_data_cmnts)
                    // {
                    // pr_cvals(vp, toget, fmt, left > toget, lastrow, (void *)vals, fsp, cor);
                    // }
                    // else
                    {
                        int len = toget;
                        for (int iel = 0; iel < len; iel++)
                        {
                            if (type == NC_FLOAT)
                                float_vals.push_back(((float *)vals)[iel]);
                            else if (type == NC_SHORT)
                                short_vals.push_back(((short *)vals)[iel]);
                            else if (type == NC_LONG)
                                long_vals.push_back(((long *)vals)[iel]);
                            else if (type == NC_BYTE)
                                byte_vals.push_back(((char *)vals)[iel]);
                        }
                    }
                    left -= toget;
                    if (vrank > 0)
                        cor[vrank - 1] += toget;
                }
                /* No failure occurs */
                if (ret >= 0)
                {
                    if (vrank > 0)
                        cor[vrank - 1] = corsav;
                    if (ir < nrows - 1)
                        if (!upcorner(vdims, ndims, cor, add))
                            printf("vardata: odometer overflowed!\n");
                    //     set_indent(2);
                }
            }

            // free(fixed_var);
            // return (err_code);
        }

        char *fmt = get_fmt(ncid, varid, type);
        int pos = 0;
        if (float_vals.size() > 0)
            json_output[std::string(fixed_str)] = float_vals; //  /*printf(" = %s",*/ dumpArray(float_vals.data(), json_output[std::string(fixed_str)], fmt, ndims, vdims, pos).c_str(); //);
        else if (short_vals.size() > 0)
            json_output[std::string(fixed_str)] = short_vals; //      /*printf(" = %s",*/ dumpArray(short_vals.data(), json_output[std::string(fixed_str)], fmt, ndims, vdims, pos).c_str(); //);
        else if (long_vals.size() > 0)
            json_output[std::string(fixed_str)] = long_vals; //      /*printf(" = %s",*/ dumpArray(long_vals.data(), json_output[std::string(fixed_str)], fmt, ndims, vdims, pos).c_str(); //);
        else if (byte_vals.size() > 0)
            json_output[std::string(fixed_str)] = byte_vals; //    /*printf(" = %s",*/ dumpArray(byte_vals.data(), json_output[std::string(fixed_str)], fmt, ndims, vdims, pos).c_str(); //);

        logger->critical("POS %d %d", float_vals.size(), pos);

        printf(";\n");

        saveCborFile(argv[2], json_output);

        free(fixed_str);
    }

    printf("\n    NLOHMANN_DEFINE_TYPE_INTRUSIVE(");
    for (int v = 0; v < vals_json_intrusive.size(); v++)
    {
        if (vals_json_intrusive.size() - 1 == v)
            printf("%s", vals_json_intrusive[v].c_str());
        else
            printf("%s, ", vals_json_intrusive[v].c_str());
        if (v % 6 == 5)
            printf("\n    ");
    }
    printf(")\n");

    printf("};\n");
}
