-- Generate a composite with
-- a cloud underlay
-- Usually used for APT imagery

function init()
    if not has_sat_proj() then
        lerror("This composite requires projection info")
        return 0
    end

    sat_proj = get_sat_proj()
    img_background = image8.new()
    img_background:load_jpeg(get_resource_path("maps/nasa_hd.jpg"))
    equ_proj = EquirectangularProj.new()
    equ_proj:init(img_background:width(), img_background:height(), -180, 90, 180, -90)
    cfg_offset = lua_vars["minoffset"]
    cfg_scalar = lua_vars["scalar"]
    cfg_thresold = lua_vars["thresold"]
    cfg_blend = lua_vars["blend"]
    cfg_invert = lua_vars["invert"]
    return 3
end

function process()
    width = rgb_output:width()
    height = rgb_output:height()
    background_size = img_background:width() * img_background:height()

    ch_equal = get_channel_image(0)
    ch_equal:equalize()

    pos = geodetic_coords_t.new()
    for x = 0, width - 1, 1 do
        for y = 0, height - 1, 1 do

            if not sat_proj:get_position(x, y, pos) then
                x2, y2 = equ_proj:forward(pos.lon, pos.lat)

                if cfg_invert == 1 then
                    val = 1.0 - ((ch_equal:get((y * width) + x) / 65535.0) - cfg_offset) * cfg_scalar
                else
                    val = ((ch_equal:get((y * width) + x) / 65535.0) - cfg_offset) * cfg_scalar
                end

                mappos = y2 * img_background:width() + x2

                if mappos >= background_size then
                    mappos = background_size - 1
                end

                if mappos < 0 then
                    mappos = 0
                end

                if cfg_blend == 1 then
                    for c = 0, 2, 1 do
                        mval = img_background:get(background_size * c + mappos) / 255.0
                        fval = mval * (1.0 - val) + val * val;
                        set_img_out(c, x, y, fval)
                    end
                else
                    if cfg_thresold == 0 then
                        for c = 0, 2, 1 do
                            mval = img_background:get(background_size * c + mappos) /
                                255.0
                            fval = mval * 0.4 + val * 0.6;
                            set_img_out(c, x, y, fval)
                        end
                    else
                        if val < cfg_thresold then
                            for c = 0, 2, 1 do
                                fval = img_background:get(background_size * c + mappos) /
                                    255.0
                                set_img_out(c, x, y, fval)
                            end
                        else
                            for c = 0, 2, 1 do
                                mval = img_background:get(background_size * c + mappos) /
                                    255.0
                                fval = mval * 0.4 + val * 0.6;
                                set_img_out(c, x, y, fval)
                            end
                        end
                    end
                end
            end
        end

        set_progress(x, width)
    end
end
