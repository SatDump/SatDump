-- Split Difference

function init()
    --load vars
    numerator = lua_vars["minval"] / lua_vars["input_range"]
    denominator = (lua_vars["maxval"] - lua_vars["minval"]) / lua_vars["input_range"]

    --create an empty image for the LUT and load
    img_lut = image_t.new()
    image_load_png(img_lut, get_resource_path(lua_vars["lut"]))

    --return 3 channels, RGB
    return 3
end

function process()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --get channels from satdump.json
            get_channel_values(x, y)
            local cch0 = get_channel_value(0)
            local cch1 = get_channel_value(1)

            --perform Difference, scaling to minval, maxval
            local difference = (cch0-cch1-numerator)/denominator


            --range convert from 0-1 to 0-255
            local lutval = difference*255

            --sanity checks
            if lutval < 0 then
                lutval = 0
            end

            if lutval > 255 then
                lutval = 255
            end

            --return RGB 0=R 1=G 2=B
            set_img_out(0, x, y, img_lut:get(lutval) / 255.0)
            set_img_out(1, x, y, img_lut:get(1 * img_lut:height() * img_lut:width() + lutval) / 255.0)
            set_img_out(2, x, y, img_lut:get(2 * img_lut:height() * img_lut:width() + lutval) / 255.0)

        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end
