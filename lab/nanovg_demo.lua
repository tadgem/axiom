local time = 0.0

local function draw_nanovg()
    local width = axiom.window.width()
    local height = axiom.window.height()
    local nvg = axiom.nvg

    local background = nvg.linear_gradient(0, 0, 0, height,
        nvg.rgba(24, 28, 42, 255), nvg.rgba(10, 12, 18, 255))
    nvg.begin_path()
    nvg.rect(0, 0, width, height)
    nvg.fill_paint(background)
    nvg.fill()

    local glow = nvg.radial_gradient(width * 0.5, height * 0.5, 20.0, width * 0.55,
        nvg.rgba(60, 130, 210, 220), nvg.rgba(60, 130, 210, 0))
    nvg.begin_path()
    nvg.rect(0, 0, width, height)
    nvg.fill_paint(glow)
    nvg.fill()

    nvg.save()
    nvg.translate(width * 0.5, height * 0.5)
    nvg.rotate(time)
    nvg.begin_path()
    nvg.circle(0, 0, 90.0)
    nvg.stroke_color(nvg.rgba(120, 220, 255, 220))
    nvg.stroke_width(6.0)
    nvg.stroke()
    nvg.restore()

    nvg.begin_path()
    nvg.rounded_rect(60, 60, 220, 120, 14.0)
    nvg.fill_color(nvg.rgba(255, 170, 60, 200))
    nvg.fill()
    nvg.begin_path()
    nvg.rounded_rect(60, 60, 220, 120, 14.0)
    nvg.stroke_color(nvg.rgba(255, 230, 180, 255))
    nvg.stroke_width(2.0)
    nvg.stroke()

    nvg.begin_path()
    nvg.rect(60, 220, 220, 120)
    nvg.circle(170.0, 280.0, 45.0)
    nvg.path_winding(nvg.winding.CW)
    nvg.fill_color(nvg.rgba(120, 255, 170, 210))
    nvg.fill()

    nvg.begin_path()
    nvg.move_to(60, 420)
    nvg.bezier_to(140, 360, 260, 480, 340, 420)
    nvg.stroke_color(nvg.rgba(255, 120, 200, 255))
    nvg.stroke_width(4.0)
    nvg.stroke()

    nvg.begin_path()
    nvg.arc(width - 220, 140, 70.0, 0.0, 4.6, nvg.winding.CW)
    nvg.fill_color(nvg.rgba(150, 140, 255, 200))
    nvg.fill()

    nvg.font_face("sans")
    nvg.font_size(28.0)
    nvg.text_align(nvg.align.CENTER + nvg.align.MIDDLE)
    nvg.fill_color(nvg.rgba(255, 255, 255, 255))
    nvg.text(width * 0.5, 100.0, "NanoVG")

    nvg.font_size(20.0)
    nvg.text_align(nvg.align.LEFT + nvg.align.TOP)
    nvg.fill_color(nvg.rgba(210, 220, 240, 255))
    nvg.text(60, height - 120, "Vector drawing: paths, gradients, arcs, beziers and text.")
    nvg.text_box(60, height - 90, 520.0, "TextBox wraps long strings across multiple lines and can be measured with text_box_bounds.")

    nvg.scissor(width - 320, height - 160, 260, 120)
    nvg.begin_path()
    nvg.circle(width - 190, height - 100, 60.0 + math.sin(time * 2.0) * 10.0)
    nvg.fill_color(nvg.rgba(255, 80, 80, 200))
    nvg.fill()
    nvg.reset_scissor()
end

function init()
    axiom.nvg.load_default_font("sans")
end

function update(dt)
    time = time + dt
end

function render()
    draw_nanovg()
end

function quit()
end
