-- Geo false color

function init()
    --Load the lut and curve
    img_lut = image8.new()
    img_curve = image8.new()
    img_lut:load_png(get_resource_path("goes/abi/wxstar/lut.png"), false)
    img_curve:load_png(get_resource_path("goes/abi/wxstar/ch2_curve.png"), false)
    --return 3 channels, RGB
    return 3
end

function process()
    lut_size = img_lut:height() * img_lut:width()
    lut_width = img_lut:width()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do
            --get channels from satdump.json
            get_channel_values(x, y)
            cha_curved = img_curve:get(get_channel_value(0) * 255.0)
            chb = get_channel_value(1) * 255.0

            --return RGB 0=R 1=G 2=B
            set_img_out(0, x, y, img_lut:get(0 * lut_size + cha_curved * lut_width + chb) / 255.0)
            set_img_out(1, x, y, img_lut:get(1 * lut_size + cha_curved * lut_width + chb) / 255.0)
            set_img_out(2, x, y, img_lut:get(2 * lut_size + cha_curved * lut_width + chb) / 255.0)
        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end
