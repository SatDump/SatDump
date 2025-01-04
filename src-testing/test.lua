-- Test


test = ImageProduct.new()
--test = loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/metop_test/AVHRR/product.cbor")

test:load("/home/alan/Downloads/SatDump_NEWPRODS/metop_test/AVHRR/product.cbor")

print("Generating " .. test.instrument_name .. " " .. test.type)

--image = generate_equation_product_composite(test, "ch3a, ch2, ch1")

--image_save_img(image, "test_lua.png")

image1 = generate_calibrated_product_channel(test, "1", 0, 50)
image2 = generate_calibrated_product_channel(test, "2", 0, 50)
image3 = generate_calibrated_product_channel(test, "3a", 0, 4)

--final_img = Image.new(image1:depth(), image1:width(), image1:height(), 3)
--final_img:draw_image(0, image3, 0, 0)
--final_img:draw_image(1, image2, 0, 0)
--final_img:draw_image(2, image1, 0, 0)

final_img = generate_equation_product_composite(test, "ch3a, ch2, ch1")
final_img:draw_image(0, image3, 0, 0)
final_img:draw_image(1, image2, 0, 0)
final_img:draw_image(2, image1, 0, 0)

Image3 = Image3.new()

--image_save_img(final_img, "test_lua.png")
