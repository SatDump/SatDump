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

    img_rainnolut = image8.new()
    img_rainnolut:load_png(get_resource_path(lua_vars["lut"]), false)

    cfg_offset = lua_vars["minoffset"]
    cfg_scalar = lua_vars["scalar"]
    cfg_thresold = lua_vars["thresold"]
    cfg_blend = lua_vars["blend"]
    return 3
end

function process()
    width = rgb_output:width()
    height = rgb_output:height()
    lut_width = img_rainnolut:width()
    lut_height = img_rainnolut:height()
    background_width = img_background:width()
    background_height = img_background:height()

    ch0_equal = get_channel_image(0)
    ch1_equal = get_channel_image(1)
    ch0_equal:equalize()
    ch1_equal:equalize()

    pos = geodetic_coords_t.new()
    for x = 0, width - 1, 1 do
        for y = 0, height - 1, 1 do

            if not sat_proj:get_position(x, y, pos) then
                x2, y2 = equ_proj:forward(pos.lon, pos.lat)

                ch1_val = ch1_equal:get((y * width) + x) / 65535.0
                lut_y = (ch1_val - cfg_offset) * cfg_scalar * lut_height
                lut_x = ((ch0_equal:get((y * width) + x) / 65535.0) - cfg_offset) * cfg_scalar * lut_width

                if lut_y >= lut_height then
                    lut_y = lut_height - 1
                end

                if lut_x >= lut_width then
                    lut_x = lut_width - 1
                end

                if lut_y < 0 then
                    lut_y = 0
                end

                if lut_x < 0 then
                    lut_x = 0
                end

                val_lut = {}
                val = {}

                val_lut[0] = img_rainnolut:get(math.floor(0 * lut_height * lut_width +
                    lut_y * lut_width + lut_x)) / 255.0
                val_lut[1] = img_rainnolut:get(math.floor(1 * lut_height * lut_width +
                    lut_y * lut_width + lut_x)) / 255.0
                val_lut[2] = img_rainnolut:get(math.floor(2 * lut_height * lut_width +
                    lut_y * lut_width + lut_x)) / 255.0

                mappos = y2 * background_width + x2
                for c = 0, 2, 1 do
                    if ch1_val > cfg_thresold then
                        mval = 0
                        this_val = val_lut[c]
                    else
                        mval = img_background:get(background_width * background_height * c + mappos) / 255.0
                        this_val = ch1_val
                    end

                    fval = mval * (1.0 - this_val) + this_val * this_val
                    set_img_out(c, x, y, fval)
                end
            end
        end

        set_progress(x, rgb_output:width())
    end
end
