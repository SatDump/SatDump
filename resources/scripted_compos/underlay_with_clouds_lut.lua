-- Generate a composite with
-- a cloud underlay
-- Usually used for APT imagery

function init()
    sat_proj = get_sat_proj()

    img_background = image8.new()
    img_background:load_jpeg(get_resource_path("maps/nasa_hd.jpg"))

    equ_proj = EquirectangularProj.new()
    equ_proj:init(img_background:width(), img_background:height(), -180, 90, 180, -90)

    img_rainnolut = image8.new()
    img_rainnolut:load_png(get_resource_path(lua_vars["lut"]))

    cfg_offset = lua_vars["minoffset"]
    cfg_scalar = lua_vars["scalar"]
    cfg_thresold = lua_vars["thresold"]
    cfg_blend = lua_vars["blend"]
    return 3
end

function process()
    pos = geodetic_coords_t.new()

    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do
            get_channel_values(x, y)

            if not sat_proj:get_position(x, y, pos) then
                x2, y2 = equ_proj:forward(pos.lon, pos.lat)


                lut_y = get_channel_value(1) * img_rainnolut:height()
                lut_x = get_channel_value(0) * img_rainnolut:width()

                if lut_y >= img_rainnolut:height() then
                    lut_y = img_rainnolut:height() - 1
                end

                if lut_x >= img_rainnolut:width() then
                    lut_x = img_rainnolut:width() - 1
                end

                if lut_y < 0 then
                    lut_y = 0
                end

                if lut_x < 0 then
                    lut_x = 0
                end

                val_lut = {}
                val = {}

                val_lut[0] = img_rainnolut:get(0 * img_rainnolut:height() * img_rainnolut:width() +
                    lut_y * img_rainnolut:width() + lut_x) / 255.0
                val_lut[1] = img_rainnolut:get(1 * img_rainnolut:height() * img_rainnolut:width() +
                    lut_y * img_rainnolut:width() + lut_x) / 255.0
                val_lut[2] = img_rainnolut:get(2 * img_rainnolut:height() * img_rainnolut:width() +
                    lut_y * img_rainnolut:width() + lut_x) / 255.0

                val[0] = (val_lut[0] - cfg_offset) * cfg_scalar
                val[1] = (val_lut[1] - cfg_offset) * cfg_scalar
                val[2] = (val_lut[2] - cfg_offset) * cfg_scalar

                mappos = y2 * img_background:width() + x2

                max_val = math.max(val[0], val[1], val[2])

                for c = 0, 2, 1 do
                    mval = img_background:get(img_background:width() * img_background:height() * c + mappos) / 255.0
                    fval = mval * (1.0 - get_channel_value(1)) + val[c] * max_val;
                    set_img_out(c, x, y, fval)
                end
            end
        end

        set_progress(x, rgb_output:width())
    end
end
