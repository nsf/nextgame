local gui = require "gui"
local M = {}

M.ScrollBox = {
	name = "ScrollBox",
	func = function(g)
		g.window:SetResizable(true)
		local TOP = "testing.scroll_box.top"
		local SCROLL_BOX = "testing.scroll_box.scroll_box"
		local CONTENTS = "testing.scroll_box.contents"
		local SLIDER = "testing.scroll_box.slider"
		local NS = "testing.scroll_box"

		local red_slider = gui.InheritStyle(gui.Slider.DefaultStyle, {
			fg = gui.Color.RED,
			fg_hover = gui.Color.RED,
		})

		g:WithAlias("w", "testing.scroll_box", function()
			local layout = ""
			for i = 1, 80 do
				g:Button("w.button"..i, {
					text = "Button #"..string.format("%02d", i),
					on_click = function()
						print("Button #"..string.format("%02d", i).." clicked, contents are: "..g:Get(NS..".edit_box"..i).text)
					end,
				})
				g:EditBox("w.edit_box"..i)
				g:Slider("w.slider"..i)
				g:Slider("w.listbox_slider"..i, {
					orientation = gui.Orientation.VERTICAL,
					on_change = function(v)
						g:Get(NS..".listbox"..i):Configure{ scroll_y = v }
					end,
					style = red_slider,
				})
				g:ListBox("w.listbox"..i, {
					list = {
						"Item A", "Item B", "Item C", "Item D", "Item E",
						"Item F", "Item G", "Item H", "Item I", "Item J",
						"Item K", "Item L", "Item M", "Item N", "Item O",
						"Item P", "Item Q", "Item R", "Item S", "Item T",
						"Item U", "Item V", "Item W", "Item X", "Item Y",
						"Item Z",
					},
				})
				layout = layout.."w.button"..i.." w.edit_box"..i.." -\n".."w.slider"..i.." - -\n".."w.listbox"..i.." - w.listbox_slider"..i.."\n"
			end
			local grid = g:Grid(CONTENTS)
			grid:Layout(layout)
			grid:ConfigureColumn(2, { weight = 1 })
			for i = 1, 80 do
				grid:Configure("w.edit_box"..i, { sticky = "WE", padx = {1, 0} })
				grid:Configure("w.slider"..i, { sticky = "WE" })
				grid:Configure("w.listbox_slider"..i, { sticky = "NS" })
				grid:Configure("w.listbox"..i, { sticky = "NWSE" })
			end

			local sb = g:ScrollBox(SCROLL_BOX, {
				virtual_size = gui.Vec2i(-1, 5),
				on_scroll_y = function(v)
					g:Get(SLIDER):ScrollY(v)
				end,
			})
			sb:SetChild(CONTENTS)

			g:Slider(SLIDER, {
				orientation = gui.Orientation.VERTICAL,
				on_change = function(v)
					g:Get(SCROLL_BOX):Configure{ scroll_y = v }
				end
			})

			local grid = g:Grid("w.top")
			grid:Layout [[
				w.scroll_box w.slider
			]]
			grid:ConfigureRow(1, { weight = 1 })
			grid:ConfigureColumn(1, { weight = 1 })
			grid:Configure("w.scroll_box", { sticky = "NWSE", padx = 1 })
			grid:Configure("w.slider", { sticky = "NS", padx = {0, 1} })
			grid:Configure("w.scroll_box w.slider", { pady = 1 })
		end)
		g:SetTop(TOP)
	end,
}

M.SpinBox = {
	name = "SpinBox",
	func = function(g)
		g.window:SetResizable(true)
		local FRAME = "testing.spin_box.frame"
		local SPINBOX = "testing.spin_box.spin_box"
		local frame = g:Frame(FRAME)
		g:SpinBox(SPINBOX)
		frame:SetChild(SPINBOX)
		g:SetTop(FRAME)
	end,
}

M.Button = {
	name = "Button",
	func = function(g)
		g.window:SetResizable(true)
		local TOP = "testing.button.top"
		local BUT1 = "testing.button.button1"
		local BUT2 = "testing.button.button2"
		local BUT3 = "testing.button.button3"
		local BUT4 = "testing.button.button4"
		local BUT5 = "testing.button.button5"
		local BUT6 = "testing.button.button6"

		local left_align = gui.InheritStyle(gui.Button.DefaultStyle, {
			align = gui.Alignment.LEFT,
		})
		local right_align = gui.InheritStyle(gui.Button.DefaultStyle, {
			align = gui.Alignment.RIGHT,
		})
		local red_bg = gui.InheritStyle(gui.Button.DefaultStyle, {
			bg = gui.Color.RED,
		})
		local red_fg = gui.InheritStyle(gui.Button.DefaultStyle, {
			fg = gui.Color.RED,
		})
		local centered_ellipsis = gui.InheritStyle(gui.Button.DefaultStyle, {
			center_ellipsis = true,
		})

		g:Button(BUT1, { text = "Default Button" })
		g:Button(BUT2, { text = "Left Alignment", style = left_align })
		g:Button(BUT3, { text = "Right Alignment", style = right_align })
		g:Button(BUT4, { text = "Red BG", style = red_bg })
		g:Button(BUT5, { text = "Red FG", style = red_fg })
		g:Button(BUT6, { text = "Centered Ellipsis", style = centered_ellipsis })
		g:WithAlias("w", "testing.button", function()
			local grid = g:Grid("w.top")
			grid:Layout [[
				w.button1 -
				w.button2 -
				w.button3 -
				w.button4 w.button5
				w.button6 -
			]]
			grid:Configure("w.button1 w.button2 w.button3 w.button4 w.button5 w.button6", { sticky = "WE" })
			grid:ConfigureColumn(1, 2, { weight = 1 })
		end)
		g:SetTop(TOP)
	end,
}

M.SliderAndProgressBar = {
	name = "Slider and ProgressBar",
	func = function(g)
		local TOP = "testing.slider_and_progress_bar.top"
		local PBH = "testing.slider_and_progress_bar.pbh"
		local PBV = "testing.slider_and_progress_bar.pbv"
		local SH  = "testing.slider_and_progress_bar.sh"
		local SV  = "testing.slider_and_progress_bar.sv"

		g:Slider(SH, {
			on_change = function(v)
				g:Get(PBH):Configure{ value = v }
			end
		})
		g:Slider(SV, {
			orientation = gui.Orientation.VERTICAL,
			value = 1,
			on_change = function(v)
				g:Get(PBV):Configure{ value = 1 - v }
			end
		})
		g:ProgressBar(PBH)
		g:ProgressBar(PBV, { orientation = gui.Orientation.VERTICAL })

		g:WithAlias("w", "testing.slider_and_progress_bar", function()
			local grid = g:Grid("w.top")
			grid:Layout [[
				w.sh  -
				w.pbh -
				w.sv  w.pbv
			]]
			grid:Configure("w.sh w.pbh w.sv w.pbv", { sticky = "NW" })

			grid:Configure("w.sh w.pbh", { padx = 1 })
			grid:Configure("w.sh", { pady = {1, 1} })
			grid:Configure("w.pbh", { pady = {0, 1} })
			grid:Configure("w.sv", { padx = {5, 1} })
			grid:Configure("w.sv w.pbv", { pady = {0, 1} })

			grid:ConfigureRow(3, { weight = 1 })
			grid:ConfigureColumn(2, { weight = 1 })
		end)
		g:SetTop(TOP)
	end,
}

M.ListBox = {
	name = "ListBox",
	func = function(g)
		local TOP    = "testing.listbox.top"
		local SLIDER = "testing.listbox.slider"
		local LB1    = "testing.listbox.lb1"
		local LB2    = "testing.listbox.lb2"
		local LB3    = "testing.listbox.lb3"

		local list1, list2, list3 = {}, {}, {}
		for i = 1, 100 do
			list1[#list1+1] = "Item "..i
			list2[#list2+1] = "Field 1: "..i
			list3[#list3+1] = "Field 2: "..i
		end

		local scroll = function(v)
			g:Get(SLIDER):ScrollY(v)
		end
		local change = function(i)
			g:Get(LB1):Configure{ selected = i }
			g:Get(LB2):Configure{ selected = i }
			g:Get(LB3):Configure{ selected = i }
		end

		g:ListBox(LB1, { list = list1, on_scroll_y = scroll, on_change = change })
		g:ListBox(LB2, { list = list2, on_scroll_y = scroll, on_change = change })
		g:ListBox(LB3, { list = list3, on_scroll_y = scroll, on_change = change })

		g:Slider(SLIDER, {
			orientation = gui.Orientation.VERTICAL,
			on_change = function(v)
				g:Get(LB1):Configure{ scroll_y = v }
				g:Get(LB2):Configure{ scroll_y = v }
				g:Get(LB3):Configure{ scroll_y = v }
			end
		})

		g:WithAlias("w", "testing.listbox", function()
			local grid = g:Grid("w.top")
			grid:Layout [[
				w.lb1 w.lb2 w.lb3 w.slider
			]]
			grid:Configure("w.lb1 w.lb2 w.lb3 w.slider", { sticky = "NWSE", pady = 1 })
			grid:Configure("w.lb1", { padx = {2, 0} })
			grid:Configure("w.slider", { padx = {0, 2} })
			grid:ConfigureRow(1, { weight = 1 })
			grid:ConfigureColumn(1, 2, 3, { weight = 1 })
		end)
		g:SetTop(TOP)
	end,
}

M.AutoResize = {
	name = "Window's AutoResize",
	func = function(g)
		local TOP        = "testing.auto_resize.top"
		local CONTENTS   = "testing.auto_resize.contents"
		local TEXT       = "testing.auto_resize.text"
		local BUTTON_ADD = "testing.auto_resize.button_add"

		local labels = {}
		g:EditBox(TEXT)
		g:Button(BUTTON_ADD, {
			text = "Add Label",
			on_click = function()
				local label_name = "testing.auto_resize.label"..(#labels+1)
				labels[#labels+1] = label_name
				g:Label(label_name, { text = g:Get(TEXT).text })
				g:Get(TEXT):Configure{ text = "" }

				local labels_layout = ""
				for _, lname in ipairs(labels) do
					labels_layout = labels_layout..lname.."\n"
				end
				local contents = g:Get(CONTENTS)
				contents:Layout(TEXT.." "..BUTTON_ADD.."\n"..labels_layout)
				contents:Configure(TEXT, { sticky = "NWSE" })
			end,
		})
		g:WithAlias("w", "testing.auto_resize", function()
			local contents = g:Grid("w.contents")
			contents:Layout [[
				w.text w.button_add
			]]
			contents:Configure("w.text", { sticky = "NWSE" })

			local grid = g:Grid("w.top")
			grid:Layout [[
				w.contents
			]]
			grid:Configure("w.contents", { sticky = "NWSE", padx = 2, pady = 1 })
		end)
		g:SetTop(TOP)
	end,
}

function M.Testing(g)
	local TOP = "testing.main.top"
	local LIST = "testing.main.list"
	local SLIDER = "testing.main.slider"
	local LAUNCH = "testing.main.launch"

	local funcs = {}
	local items = {}
	for k, v in pairs(M) do
		if type(v) == "table" then
			items[#items+1] = v.name
			funcs[#funcs+1] = v.func
		end
	end

	local grid = g:Grid(TOP)
	g:Slider(SLIDER, {
		orientation = gui.Orientation.VERTICAL,
		on_change = function(v)
			g:Get(LIST):Configure{ scroll_y = v }
		end
	})
	g:ListBox(LIST, {
		list = items,
		on_scroll_y = function(v)
			g:Get(SLIDER):ScrollY(v)
		end,
	})
	g:Button(LAUNCH, {
		text = "Launch",
		on_click = function()
			if not funcs[g:Get(LIST).selected] then
				return
			end
			local ng = gui.AddWindow(gui.Vec2i(5, 5), gui.WindowAlignment.FLOATING, true)
			funcs[g:Get(LIST).selected](ng)
			ng:RedrawMaybe()
		end,
	})

	g:WithAlias("w", "testing.main", function()
		grid:Layout [[
			w.list w.slider
			w.launch -
		]]
		grid:Configure("w.list", { sticky = "NWSE" })
		grid:Configure("w.slider", { sticky = "NS" })
		grid:Configure("w.launch", { pady = 1 })
		grid:ConfigureColumn(1, { weight = 1 })
		grid:ConfigureRow(1, { weight = 1 })
	end)
	g:SetTop(TOP)
end

return M
