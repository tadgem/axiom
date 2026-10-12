local state = {
    slider = 0.35,
    int = 12,
    drag = 1.5,
    drag3 = { 1.0, 2.0, 3.0 },
    check = true,
    radio = 1,
    input = "edit me",
    number = 3.14,
    count = 4,
    combo = 1,
    list = 2,
    selectable = 1,
    color = { 0.2, 0.6, 1.0 },
    color4 = { 0.2, 0.6, 1.0, 1.0 },
    progress = 0.5
}

local items = { "Alpha", "Beta", "Gamma", "Delta", "Epsilon" }
local history = {}

for i = 1, 128 do
    history[i] = math.sin(i * 0.15) * 0.5 + math.cos(i * 0.05) * 0.5
end

local widgets = nil

local function torque_ui()
    local imgui = axiom.imgui

    imgui.begin("ImGui Features")

    if imgui.collapsing_header("Text") then
        imgui.text("Plain text")
        imgui.text_colored(1.0, 0.65, 0.2, 1.0, "Colored text")
        imgui.text_disabled("Disabled text")
        imgui.text_wrapped("Wrapped text that will automatically flow onto the next line when it exceeds the available width.")
        imgui.bullet_text("Bullet point")
        imgui.label_text("Label", "value")
    end

    if imgui.collapsing_header("Buttons") then
        if imgui.button("Primary Button") then
            log("primary button pressed")
        end
        imgui.same_line()
        if imgui.small_button("Small") then
            log("small button pressed")
        end
        imgui.same_line()
        if imgui.arrow_button("##arrow", 0) then
            log("arrow button pressed")
        end
    end

    if imgui.collapsing_header("Inputs") then
        state.slider = imgui.slider_float("Slider", state.slider, 0.0, 1.0)
        state.int = imgui.slider_int("Slider Int", state.int, 0, 64)
        state.drag = imgui.drag_float("Drag", state.drag)
        state.drag3[1], state.drag3[2], state.drag3[3] = imgui.drag_float3("Drag 3", state.drag3[1], state.drag3[2], state.drag3[3])
        state.number = imgui.input_float("Input Float", state.number)
        state.input = imgui.input_text("Input Text", state.input)

        local changed, value = imgui.checkbox("Checkbox", state.check)
        state.check = value
        if changed then
            log(string.format("checkbox changed to %s", tostring(value)))
        end

        if imgui.radio_button("Radio A", state.radio == 1) then
            state.radio = 1
        end
        imgui.same_line()
        if imgui.radio_button("Radio B", state.radio == 2) then
            state.radio = 2
        end
    end

    if imgui.collapsing_header("Selection") then
        state.combo = imgui.combo("Combo", state.combo, items)
        state.list = imgui.list_box("List Box", state.list, items)

        for index = 1, #items do
            if imgui.selectable(items[index], state.selectable == index) then
                state.selectable = index
            end
        end
    end

    if imgui.collapsing_header("Color") then
        state.color[1], state.color[2], state.color[3] = imgui.color_edit3("Color", state.color[1], state.color[2], state.color[3])
        state.color4[1], state.color4[2], state.color4[3], state.color4[4] =
            imgui.color_edit4("Color Alpha", state.color4[1], state.color4[2], state.color4[3], state.color4[4])
    end

    if imgui.collapsing_header("Plots and Progress") then
        imgui.progress_bar(state.progress)
        state.progress = imgui.slider_float("Progress", state.progress, 0.0, 1.0)
        imgui.plot_lines("Signal", history)
        imgui.plot_histogram("Spectrum", history)
    end

    if imgui.tree_node("Tree") then
        imgui.text("Inside the tree")
        if imgui.tree_node("Nested") then
            imgui.text("Nested content")
            imgui.tree_pop()
        end
        imgui.tree_pop()
    end

    imgui.separator()
    imgui.text(string.format("fps %.1f", imgui.framerate()))

    imgui.finish()

    imgui.set_next_window_pos(460, 20)
    imgui.begin("Secondary Window")
    imgui.text("This is a second window.")
    local width, height = imgui.get_content_region_avail()
    imgui.text(string.format("Available %.0f x %.0f", width, height))
    imgui.finish()
end

function init()
    widgets = torque_ui
end

function update(dt)
    widgets()
end

function render()
end

function quit()
end
