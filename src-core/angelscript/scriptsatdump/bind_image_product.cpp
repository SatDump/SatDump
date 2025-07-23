#include "angelscript/scriptarray/scriptarray.h"
#include "bind_satdump.h"

#include "products/image/product_expression.h"
#include "products/image_product.h"
#include "products/product.h"
#include "projection/projection.h"

#include "bind_geodetic.h"
#include "bind_json.h"

#include "bind_image.h"
#include "bind_product.h"
#include <memory>

namespace satdump
{
    namespace script
    {
        namespace
        {
            class CScriptImageHolder
            {
            };

            class CScriptImageProduct : public CScriptProduct
            {
            public:
                // Constructors
                CScriptImageProduct()
                {
                    refCount = 1;
                    CScriptProduct::p = std::make_shared<products::ImageProduct>();
                    j = new CScriptJson(&p->contents);
                    this->_p = (products::ImageProduct *)CScriptProduct::p.get();
                }

            public:
                products::ImageProduct *_p;

            public:
                void set_proj_cfg(CScriptJson *c) { _p->set_proj_cfg(*c->j); }
                void set_proj_cfg_tle_timestamps(CScriptJson *c, CScriptJson *tl, CScriptJson *time) { _p->set_proj_cfg_tle_timestamps(*c->j, *tl->j, *time->j); }
                CScriptJson *get_proj_cfg(int channel) { return new CScriptJson(_p->get_proj_cfg(channel)); }
                bool has_proj_cfg() { return _p->has_proj_cfg(); }

                void set_calibration(std::string n, CScriptJson *c) { return _p->set_calibration(n, *c->j); }
                bool has_calibration() { return _p->has_calibration(); }
                CScriptJson *get_calibration_raw() { return new CScriptJson(_p->get_calibration_raw()); }

                int get_channel_image_idx(std::string name) { return _p->get_channel_image_idx(name); }
                int get_raw_channel_val(int i, int x, int y) { return _p->get_raw_channel_val(i, x, y); }

                void set_channel_wavenumber(int i, double v) { return _p->set_channel_wavenumber(i, v); }
                void set_channel_frequency(int i, double v) { return _p->set_channel_frequency(i, v); }
                double get_channel_wavenumber(int i) { return _p->get_channel_wavenumber(i); }
                double get_channel_frequency(int i) { return _p->get_channel_frequency(i); }
                void set_channel_unit(int i, std::string u) { _p->set_channel_unit(i, u); }

            public:
                CScriptProduct *opConv_product() { return this; }
            };
        } // namespace

        namespace exp
        {
            bool check_expression_product_composite(CScriptImageProduct *product, std::string e) { return products::check_expression_product_composite(product->_p, e); }

            CScriptImage *generate_expression_product_composite(CScriptImageProduct *product, std::string e)
            {
                return new CScriptImage(products::generate_expression_product_composite(product->_p, e));
            }
        } // namespace exp

        void registerImageProduct(asIScriptEngine *engine)
        {
            ////////////////////////////////
            //////// Image Holder
            ////////////////////////////////

            // TODOREWORK image holder

            ////////////////////////////////
            //////// Image Product Class
            ////////////////////////////////

            registerProduct<CScriptImageProduct>("ImageProduct", engine);

            // Copy operator
            engine->RegisterObjectMethod("ImageProduct", "Product@ opImplConv()", asMETHOD(CScriptImageProduct, opConv_product), asCALL_THISCALL);
            // TODOREWORK engine->RegisterObjectMethod("Product", "ImageProduct@ opImplConv()", asMETHOD(CScriptImageProduct, opConv_product), asCALL_THISCALL);

            // Other funcs
            engine->RegisterObjectMethod("ImageProduct", "void set_proj_cfg(nlohmann::json@)", asMETHOD(CScriptImageProduct, set_proj_cfg), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "void set_proj_cfg_tle_timestamps(nlohmann::json@, nlohmann::json@, nlohmann::json@)",
                                         asMETHOD(CScriptImageProduct, set_proj_cfg_tle_timestamps), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "nlohmann::json@ get_proj_cfg(int)", asMETHOD(CScriptImageProduct, get_proj_cfg), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "bool has_proj_cfg()", asMETHOD(CScriptImageProduct, has_proj_cfg), asCALL_THISCALL);

            engine->RegisterObjectMethod("ImageProduct", "void set_calibration(string, nlohmann::json@)", asMETHOD(CScriptImageProduct, set_calibration), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "bool has_calibration()", asMETHOD(CScriptImageProduct, has_calibration), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "nlohmann::json@ get_calibration_raw()", asMETHOD(CScriptImageProduct, get_calibration_raw), asCALL_THISCALL);

            // TODOREWORK get_channel_image
            engine->RegisterObjectMethod("ImageProduct", "int get_channel_image_idx(string &in)", asMETHOD(CScriptImageProduct, get_channel_image_idx), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "int get_raw_channel_val(int, int, int)", asMETHOD(CScriptImageProduct, get_raw_channel_val), asCALL_THISCALL);

            engine->RegisterObjectMethod("ImageProduct", "void set_channel_wavenumber(int, double)", asMETHOD(CScriptImageProduct, set_channel_wavenumber), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "void set_channel_frequency(int, double)", asMETHOD(CScriptImageProduct, set_channel_frequency), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "double get_channel_wavenumber(int)", asMETHOD(CScriptImageProduct, get_channel_wavenumber), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "double get_channel_frequency(int)", asMETHOD(CScriptImageProduct, get_channel_frequency), asCALL_THISCALL);
            engine->RegisterObjectMethod("ImageProduct", "void set_channel_unit(int, string)", asMETHOD(CScriptImageProduct, set_channel_unit), asCALL_THISCALL);

            // Values
            engine->RegisterObjectProperty("ImageProduct", "bool save_as_matrix", asOFFSET(products::ImageProduct, save_as_matrix), asOFFSET(CScriptImageProduct, p), true);
            engine->RegisterObjectProperty("ImageProduct", "bool d_no_not_save_images", asOFFSET(products::ImageProduct, d_no_not_save_images), asOFFSET(CScriptImageProduct, p), true);
            engine->RegisterObjectProperty("ImageProduct", "bool d_no_not_load_images", asOFFSET(products::ImageProduct, d_no_not_load_images), asOFFSET(CScriptImageProduct, p), true);

            ////////////////////////////////
            //////// Expression
            ////////////////////////////////

            engine->RegisterGlobalFunction("bool check_expression_product_composite(ImageProduct&, string)", asFUNCTION(exp::check_expression_product_composite), asCALL_CDECL);
            engine->RegisterGlobalFunction("image::Image@ generate_expression_product_composite(ImageProduct&, string)", asFUNCTION(exp::generate_expression_product_composite), asCALL_CDECL);
        }
    } // namespace script
} // namespace satdump