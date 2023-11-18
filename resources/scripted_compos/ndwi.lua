-- NDVI

function init()
    --create an empty image for the LUT
    img_lut = image8.new()
    --load the LUT
    img_lut:load_png(get_resource_path("lut/NDWI.png"))
    --return 3 channels, RGB
    return 3
end

function process()
    for x = 0, rgb_output:width() - 1, 1 do
        for y = 0, rgb_output:height() - 1, 1 do

            --get channels from satdump.json

            local cch2 = get_calibrated_value(1, x, y, true)
            local cch3a = get_calibrated_value(2, x, y, true)
            

            --perform NDVI
            local ndwi = (cch2-cch3a)/(cch2+cch3a)

            --range convert from -1 -> 1 to 0 -> 256
            local ndwi_lutval = 256 - ((ndwi-(-1))*256)/(1-(-1))+0

            --don't forget to create a table!
            local lut_result = {}
        
            --send the ndwi_lutval to the LUT, retrieve it back in the table as 3 R,G,B channels
            lut_result[0] = img_lut:get(0 * img_lut:height() * img_lut:width() + ndwi_lutval * img_lut:width() + ndwi_lutval) / 255.0
            lut_result[1] = img_lut:get(1 * img_lut:height() * img_lut:width() + ndwi_lutval * img_lut:width() + ndwi_lutval) / 255.0
            lut_result[2] = img_lut:get(2 * img_lut:height() * img_lut:width() + ndwi_lutval * img_lut:width() + ndwi_lutval) / 255.0
            
            --return RGB 0=R 1=G 2=B
            set_img_out(0, x, y, lut_result[0])
            set_img_out(1, x, y, lut_result[1])
            set_img_out(2, x, y, lut_result[2])
            
        end
        --set the progress bar accordingly
        set_progress(x, rgb_output:width())
    end
end
