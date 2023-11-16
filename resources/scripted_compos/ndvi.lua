-- NDVI

function init()
    img_lut = image8.new()
    img_lut:load_png(get_resource_path("lut/NDVI.png"))
    return 3
end

function process()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do
            local cch2 = get_calibrated_value(0, x, y, true)
            local cch1 = get_calibrated_value(1, x, y, true)



            local ndvi = (cch2-cch1)/(cch2+cch1)

            local ndvi_lutval = ((ndvi-(-1))*256)/(1-(-1))+0 
            
            local lut_result[0] = img_lut:get(0 * img_lut:height() * img_lut:width() + ndvi_lutval) / 255.0
            local lut_result[1] = img_lut:get(1 * img_lut:height() * img_lut:width() + ndvi_lutval) / 255.0
            local lut_result[2] = img_lut:get(2 * img_lut:height() * img_lut:width() + ndvi_lutval) / 255.0

            for c = 0, 2, 1 do
                set_img_out(c, x, y, lut_result[c])
            end
        end
        set_progress(x, rgb_output:width())
    end
end
