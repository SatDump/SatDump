local function clamp01(x)
    if x < 0 then return 0 end
    if x > 1 then return 1 end
    return x
end

-- Box blur
local function box_blur(src, W, H, r)
    if r <= 0 then
        local out = {}
        for i = 1, #src do out[i] = src[i] end
        return out
    end
    local win = 2 * r + 1
    local inv_win = 1.0 / win

    -- horizontal pass
    local tmp = {}
    for y = 0, H - 1 do
        local base = y * W
        local function valx(x)
            if x < 0 then x = 0 elseif x > W - 1 then x = W - 1 end
            return src[base + x + 1]
        end
        local sum = 0.0
        for i = -r, r do sum = sum + valx(i) end
        tmp[base + 1] = sum * inv_win
        for x = 1, W - 1 do
            sum = sum + valx(x + r) - valx(x - r - 1)
            tmp[base + x + 1] = sum * inv_win
        end
    end

    -- vertical pass
    local out = {}
    for x = 0, W - 1 do
        local function valy(y)
            if y < 0 then y = 0 elseif y > H - 1 then y = H - 1 end
            return tmp[y * W + x + 1]
        end
        local sum = 0.0
        for i = -r, r do sum = sum + valy(i) end
        out[x + 1] = sum * inv_win
        for y = 1, H - 1 do
            sum = sum + valy(y + r) - valy(y - r - 1)
            out[y * W + x + 1] = sum * inv_win
        end
    end
    return out
end

local function pct_thresholds(hist, bins, clip)
    local total = 0
    for i = 1, bins do total = total + hist[i] end
    if total == 0 then return 0.0, 1.0 end

    local lo_t = clip * total
    local hi_t = (1.0 - clip) * total

    local cum, lo_i = 0, 1
    for i = 1, bins do
        cum = cum + hist[i]
        if cum >= lo_t then lo_i = i; break end
    end
    cum = 0
    local hi_i = bins
    for i = 1, bins do
        cum = cum + hist[i]
        if cum >= hi_t then hi_i = i; break end
    end

    local scale = bins - 1
    local lo = (lo_i - 1) / scale
    local hi = (hi_i - 1) / scale
    if hi <= lo then return 0.0, 1.0 end
    return lo, hi
end

-- Params
function init()
    ch4_gamma    = lua_vars["ch4_gamma"]    or 0.4
    red_gamma    = lua_vars["red_gamma"]    or 0.8
    blur_size    = lua_vars["blur_size"]    or 780
    hist_stretch = lua_vars["hist_stretch"] or 0.01
    hist_bins    = 256
    hist_scale   = 255
    hist_thres   = 1.0 / 255.0

    return 3
end

-- Main
function process()
    local W = rgb_output:width()
    local H = rgb_output:height()
    local r = blur_size

    -- 1) ch4 gamma
    local inv_c4_gamma = 1.0 / ch4_gamma
    local ch4_gamma_buf = {}
    local k = 0
    for y = 0, H - 1 do
        for x = 0, W - 1 do
            get_channel_values(x, y)
            local c4 = get_channel_value(0)
            k = k + 1
            ch4_gamma_buf[k] = clamp01(c4) ^ inv_c4_gamma
        end
        if (y & 31) == 0 then set_progress(y, H) end
    end

    -- 2) ch4 blur
    local ch4_blur = box_blur(ch4_gamma_buf, W, H, r)

    -- 3) grain ops + hist build
    local Rbuf, Gbuf, Bbuf = {}, {}, {}
    local hR, hG, hB = {}, {}, {}
    for i = 1, hist_bins do hR[i] = 0; hG[i] = 0; hB[i] = 0 end

    k = 0
    for y = 0, H - 1 do
        for x = 0, W - 1 do
            k = k + 1
            local ge = clamp01(ch4_gamma_buf[k] - ch4_blur[k] + 0.5)

            get_channel_values(x, y)
            local c2 = get_channel_value(1)
            local c1 = get_channel_value(2)

            local gm = clamp01(ge + c2 - 0.5)

            Rbuf[k], Gbuf[k], Bbuf[k] = gm, c2, c1

            if gm > hist_thres then
                local iR = math.floor(gm * hist_scale + 0.5) + 1
                if iR < 1 then iR = 1 elseif iR > hist_bins then iR = hist_bins end
                hR[iR] = hR[iR] + 1
            end
            if c2 > hist_thres then
                local iG = math.floor(c2 * hist_scale + 0.5) + 1
                if iG < 1 then iG = 1 elseif iG > hist_bins then iG = hist_bins end
                hG[iG] = hG[iG] + 1
            end
            if c1 > hist_thres then
                local iB = math.floor(c1 * hist_scale + 0.5) + 1
                if iB < 1 then iB = 1 elseif iB > hist_bins then iB = hist_bins end
                hB[iB] = hB[iB] + 1
            end
        end
        if (y & 31) == 0 then set_progress(y, H) end
    end

    -- 4) percentile stretch per channel
    local loR, hiR = pct_thresholds(hR, hist_bins, hist_stretch)
    local loG, hiG = pct_thresholds(hG, hist_bins, hist_stretch)
    local loB, hiB = pct_thresholds(hB, hist_bins, hist_stretch)
    
    local function inv_span(lo, hi)
        local d = hi - lo
        if d <= 1e-12 then return 1.0 end
        return 1.0 / d
    end
    
    local invR = inv_span(loR, hiR)
    local invG = inv_span(loG, hiG)
    local invB = inv_span(loB, hiB)
    local inv_red = 1.0 / red_gamma
    
    k = 0
    for y = 0, H - 1 do
        for x = 0, W - 1 do
            k = k + 1
            local r_out = (Rbuf[k] - loR) * invR
            local g_out = (Gbuf[k] - loG) * invG
            local b_out = (Bbuf[k] - loB) * invB

            r_out = clamp01(r_out) ^ inv_red
            g_out = clamp01(g_out)
            b_out = clamp01(b_out)

            set_img_out(0, x, y, r_out)
            set_img_out(1, x, y, g_out)
            set_img_out(2, x, y, b_out)
        end
        if (y & 31) == 0 then set_progress(y, H) end
    end
end
