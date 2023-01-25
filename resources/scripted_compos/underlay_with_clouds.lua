-- Generate a composite with
-- a cloud underlay
-- Usually used for APT imagery

function init()
    sat_proj = get_sat_proj()
    img_background = image8.new()
    img_background:load_jpeg(get_resource_path("maps/nasa_hd.jpg"))
    equ_proj = EquirectangularProj.new()
    equ_proj:init(img_background:width(), img_background:height(), -180, 90, 180, -90)
    cfg_offset = lua_vars["minoffset"]
    cfg_scalar = lua_vars["scalar"]
    cfg_thresold = lua_vars["thresold"]
    return 3
end

function process()

    pos = geodetic_coords_t.new()

    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do
            get_channel_values(x, y)

            if not sat_proj:get_position(x, y, pos) then
                x2, y2 = equ_proj:forward(pos.lon, pos.lat)

                val = (get_channel_value(0) - cfg_offset) * cfg_scalar

                mappos = y2 * img_background:width() + x2

                if cfg_thresold == 0 then
                    for c = 0, 2, 1 do
                        mval = img_background:get(img_background:width() * img_background:height() * c + mappos) / 255.0
                        fval = mval * 0.4 + val * 0.6;
                        set_img_out(c, x, y, fval)
                    end
                else
                    if val < cfg_thresold then
                        for c = 0, 2, 1 do
                            fval = img_background:get(img_background:width() * img_background:height() * c + mappos) /
                                255.0
                            set_img_out(c, x, y, fval)
                        end
                    else
                        for c = 0, 2, 1 do
                            mval = img_background:get(img_background:width() * img_background:height() * c + mappos) /
                                255.0
                            fval = mval * 0.4 + val * 0.6;
                            set_img_out(c, x, y, fval)
                        end
                    end
                end
            end
        end

        set_progress(x, rgb_output:width())
    end
end
