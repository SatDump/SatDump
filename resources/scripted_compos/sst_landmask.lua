-- Generate a SST Product
-- with land blacked out
-- Should work with any calibrated
-- radiometer

function init()
    if not has_sat_proj() then
        lerror("This composite requires projection info")
        return 0
    end

    sat_proj = get_sat_proj()
    img_landmask = image8.new()
    img_landmask:load_jpeg(get_resource_path("maps/landmask.jpg"))
    equ_proj = EquirectangularProj.new()
    equ_proj:init(img_landmask:width(), img_landmask:height(), -180, 90, 180, -90)
    cfg_maxval = lua_vars["maxval"]
    cfg_minval = lua_vars["minval"]
    return 3
end

function process()
    pos = geodetic_coords_t.new()

    jetlut = image8_lut_jet()
    -- get_calibrated_image(3, "temperature", 278.2, 292.0)

    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do
            get_channel_values(x, y)

            if not sat_proj:get_position(x, y, pos) then
                x2, y2 = equ_proj:forward(pos.lon, pos.lat)

                mappos = y2 * img_landmask:width() + x2

                mval = img_landmask:get(img_landmask:width() * img_landmask:height() + mappos) / 255.0


                if mval < 0.1 then
                    val = get_channel_value(0)

                    lutpos = val * 255

                    if lutpos >= jetlut:width() then
                        lutpos = jetlut:width() - 1
                    end

                    if lutpos < 0 then
                        lutpos = 0
                    end

                    set_img_out(0, x, y, jetlut:get(jetlut:width() * 0 + lutpos) / 255.0)
                    set_img_out(1, x, y, jetlut:get(jetlut:width() * 1 + lutpos) / 255.0)
                    set_img_out(2, x, y, jetlut:get(jetlut:width() * 2 + lutpos) / 255.0)
                end
            end
        end

        set_progress(x, rgb_output:width())
    end
end
