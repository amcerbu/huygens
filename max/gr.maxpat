{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 8,
			"minor" : 5,
			"revision" : 2,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 34.0, 106.0, 1302.0, 592.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-105",
					"linecount" : 3,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 207.024319289252162, 491.038752544487068, 61.0, 47.0 ],
					"text" : "toggle keyboard shortcuts"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-103",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 178.0, 467.038752544487068, 70.0, 22.0 ],
					"text" : "loadmess 1"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-102",
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 178.0, 491.038752544487068, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-330",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 121.5, 491.038752544487068, 53.0, 22.0 ],
					"text" : "s locked"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-326",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 16.0, 541.038752544487124, 89.0, 22.0 ],
					"text" : "s keycommand"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-136",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 16.0, 517.038752544487124, 181.0, 22.0 ],
					"text" : "if ($i2 == 1 && $i3 == 1) then $i1"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-78",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 4,
					"outlettype" : [ "int", "int", "int", "int" ],
					"patching_rect" : [ 16.0, 493.038752544487068, 50.5, 22.0 ],
					"text" : "key"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-76",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 69.0, 467.038752544487068, 58.0, 22.0 ],
					"text" : "loadbang"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-72",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 69.0, 491.038752544487068, 29.5, 22.0 ],
					"text" : "poll"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-65",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 100.5, 491.038752544487068, 19.0, 22.0 ],
					"saved_object_attributes" : 					{
						"filename" : "lockstate.js",
						"parameter_enable" : 0
					}
,
					"text" : "js"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-19",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 195.0, 15.0, 102.0, 20.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 195.0, 15.0, 102.0, 20.0 ],
					"text" : "send thru params"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-16",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 465.0, 15.0, 54.0, 22.0 ],
					"text" : "delay 60"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-15",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 405.0, 15.0, 54.0, 22.0 ],
					"text" : "delay 40"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-14",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 345.0, 15.0, 54.0, 22.0 ],
					"text" : "delay 20"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-6",
					"maxclass" : "live.button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 180.0, 15.0, 15.0, 15.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 180.0, 15.0, 15.0, 15.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "off", "on" ],
							"parameter_longname" : "live.button[1]",
							"parameter_mmax" : 1,
							"parameter_shortname" : "live.button",
							"parameter_type" : 2
						}

					}
,
					"varname" : "live.button[1]"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-35",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 15.0, 97.0, 20.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 30.0, 15.0, 97.0, 20.0 ],
					"text" : "toggle animation"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-34",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 75.0, 405.0, 291.0, 20.0 ],
					"text" : "label data by voice number; small delay at patch boot"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-32",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 5,
							"revision" : 2,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "box",
						"rect" : [ 59.0, 104.0, 640.0, 480.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"assistshowspatchername" : 0,
						"boxes" : [ 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-2",
									"index" : 2,
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 178.0, 307.162108999999987, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-15",
									"maxclass" : "message",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 80.0, 220.0, 29.5, 22.0 ],
									"text" : "1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-13",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 50.0, 190.0, 61.0, 22.0 ],
									"text" : "delay 500"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-12",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 50.0, 160.0, 58.0, 22.0 ],
									"text" : "loadbang"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-6",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 170.0, 220.0, 70.0, 22.0 ],
									"text" : "loadmess 0"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-5",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 125.0, 220.0, 32.0, 22.0 ],
									"text" : "gate"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-11",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 260.0, 130.0, 76.0, 22.0 ],
									"text" : "o.prepend /4"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-10",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 245.0, 100.0, 76.0, 22.0 ],
									"text" : "o.prepend /3"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-9",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 170.0, 130.0, 76.0, 22.0 ],
									"text" : "o.prepend /2"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-8",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 155.0, 100.0, 76.0, 22.0 ],
									"text" : "o.prepend /1"
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-27",
									"index" : 1,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 155.0, 40.0, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-28",
									"index" : 2,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 190.0, 40.0, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-29",
									"index" : 3,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 245.0, 40.0, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-30",
									"index" : 4,
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "FullPacket" ],
									"patching_rect" : [ 280.0, 40.0, 30.0, 30.0 ]
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-31",
									"index" : 1,
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 125.0, 307.162108999999987, 30.0, 30.0 ]
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 1 ],
									"source" : [ "obj-10", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 1 ],
									"source" : [ "obj-11", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-13", 0 ],
									"source" : [ "obj-12", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-15", 0 ],
									"order" : 1,
									"source" : [ "obj-13", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"order" : 0,
									"source" : [ "obj-13", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 0 ],
									"source" : [ "obj-15", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-8", 0 ],
									"source" : [ "obj-27", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 0 ],
									"source" : [ "obj-28", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-10", 0 ],
									"source" : [ "obj-29", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-11", 0 ],
									"source" : [ "obj-30", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-31", 0 ],
									"source" : [ "obj-5", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 0 ],
									"source" : [ "obj-6", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 1 ],
									"source" : [ "obj-8", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 1 ],
									"source" : [ "obj-9", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 15.0, 405.0, 50.5, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"tags" : ""
					}
,
					"text" : "p"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-18",
					"maxclass" : "live.button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"parameter_enable" : 1,
					"patching_rect" : [ 15.0, 15.0, 15.0, 15.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 15.0, 15.0, 15.0, 15.0 ],
					"saved_attribute_attributes" : 					{
						"valueof" : 						{
							"parameter_enum" : [ "off", "on" ],
							"parameter_longname" : "live.button",
							"parameter_mmax" : 1,
							"parameter_shortname" : "live.button",
							"parameter_type" : 2
						}

					}
,
					"varname" : "live.button"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-555",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 15.0, 435.0, 138.0, 22.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 15.0, 390.0, 138.0, 22.0 ],
					"text" : "udpsend 127.0.0.1 9000"
				}

			}
, 			{
				"box" : 				{
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "obj-4",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "osc_granny.maxpat",
					"numinlets" : 3,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "FullPacket" ],
					"patching_rect" : [ 415.0, 45.0, 400.0, 170.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 415.0, 45.0, 400.0, 170.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "obj-3",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "osc_granny.maxpat",
					"numinlets" : 3,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "FullPacket" ],
					"patching_rect" : [ 415.0, 215.0, 400.0, 170.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 415.0, 215.0, 400.0, 170.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "obj-2",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "osc_granny.maxpat",
					"numinlets" : 3,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "FullPacket" ],
					"patching_rect" : [ 15.0, 215.0, 400.0, 170.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 15.0, 215.0, 400.0, 170.0 ],
					"viewvisibility" : 1
				}

			}
, 			{
				"box" : 				{
					"bgmode" : 0,
					"border" : 0,
					"clickthrough" : 0,
					"enablehscroll" : 0,
					"enablevscroll" : 0,
					"id" : "obj-1",
					"lockeddragscroll" : 0,
					"lockedsize" : 0,
					"maxclass" : "bpatcher",
					"name" : "osc_granny.maxpat",
					"numinlets" : 3,
					"numoutlets" : 1,
					"offset" : [ 0.0, 0.0 ],
					"outlettype" : [ "FullPacket" ],
					"patching_rect" : [ 15.0, 45.0, 400.0, 170.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 15.0, 45.0, 400.0, 170.0 ],
					"viewvisibility" : 1
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 0 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-136", 2 ],
					"source" : [ "obj-102", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-102", 0 ],
					"source" : [ "obj-103", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-326", 0 ],
					"source" : [ "obj-136", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-14", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"source" : [ "obj-15", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-3", 0 ],
					"source" : [ "obj-16", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 2 ],
					"order" : 3,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 2 ],
					"order" : 2,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-3", 2 ],
					"order" : 1,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 2 ],
					"order" : 0,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 1 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 3 ],
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-555", 0 ],
					"source" : [ "obj-32", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 0 ],
					"source" : [ "obj-32", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 2 ],
					"source" : [ "obj-4", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"order" : 3,
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-14", 0 ],
					"order" : 2,
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-15", 0 ],
					"order" : 1,
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-16", 0 ],
					"order" : 0,
					"source" : [ "obj-6", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-136", 1 ],
					"order" : 1,
					"source" : [ "obj-65", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-330", 0 ],
					"order" : 0,
					"source" : [ "obj-65", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-65", 0 ],
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-72", 0 ],
					"source" : [ "obj-76", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-136", 0 ],
					"source" : [ "obj-78", 0 ]
				}

			}
 ],
		"parameters" : 		{
			"obj-18" : [ "live.button", "live.button", 0 ],
			"obj-1::obj-117" : [ "active", "active", 0 ],
			"obj-1::obj-123" : [ "gain", "gain", 0 ],
			"obj-1::obj-125" : [ "texture", "texture", 0 ],
			"obj-1::obj-128" : [ "mixitup", "mixitup", 0 ],
			"obj-1::obj-171" : [ "pan", "pan", 0 ],
			"obj-1::obj-176" : [ "shape", "shape", 0 ],
			"obj-1::obj-177" : [ "transp", "transp", 0 ],
			"obj-1::obj-179" : [ "size", "size", 0 ],
			"obj-1::obj-180" : [ "density", "density", 0 ],
			"obj-1::obj-181" : [ "scatter", "scatter", 0 ],
			"obj-1::obj-182" : [ "spray", "spray", 0 ],
			"obj-1::obj-183" : [ "jitter", "jitter", 0 ],
			"obj-1::obj-184" : [ "wobble", "wobble", 0 ],
			"obj-1::obj-185" : [ "warble", "warble", 0 ],
			"obj-1::obj-33" : [ "mixitup[1]", "mixitup", 0 ],
			"obj-1::obj-40" : [ "reverse", "reverse", 0 ],
			"obj-2::obj-117" : [ "active[1]", "active", 0 ],
			"obj-2::obj-123" : [ "gain[1]", "gain", 0 ],
			"obj-2::obj-125" : [ "texture[1]", "texture", 0 ],
			"obj-2::obj-128" : [ "mixitup[3]", "mixitup", 0 ],
			"obj-2::obj-171" : [ "pan[1]", "pan", 0 ],
			"obj-2::obj-176" : [ "shape[1]", "shape", 0 ],
			"obj-2::obj-177" : [ "transp[1]", "transp", 0 ],
			"obj-2::obj-179" : [ "size[1]", "size", 0 ],
			"obj-2::obj-180" : [ "density[1]", "density", 0 ],
			"obj-2::obj-181" : [ "scatter[1]", "scatter", 0 ],
			"obj-2::obj-182" : [ "spray[1]", "spray", 0 ],
			"obj-2::obj-183" : [ "jitter[1]", "jitter", 0 ],
			"obj-2::obj-184" : [ "wobble[1]", "wobble", 0 ],
			"obj-2::obj-185" : [ "warble[1]", "warble", 0 ],
			"obj-2::obj-33" : [ "mixitup[2]", "mixitup", 0 ],
			"obj-2::obj-40" : [ "reverse[1]", "reverse", 0 ],
			"obj-3::obj-117" : [ "active[2]", "active", 0 ],
			"obj-3::obj-123" : [ "gain[2]", "gain", 0 ],
			"obj-3::obj-125" : [ "texture[2]", "texture", 0 ],
			"obj-3::obj-128" : [ "mixitup[4]", "mixitup", 0 ],
			"obj-3::obj-171" : [ "pan[2]", "pan", 0 ],
			"obj-3::obj-176" : [ "shape[2]", "shape", 0 ],
			"obj-3::obj-177" : [ "transp[2]", "transp", 0 ],
			"obj-3::obj-179" : [ "size[2]", "size", 0 ],
			"obj-3::obj-180" : [ "density[2]", "density", 0 ],
			"obj-3::obj-181" : [ "scatter[2]", "scatter", 0 ],
			"obj-3::obj-182" : [ "spray[2]", "spray", 0 ],
			"obj-3::obj-183" : [ "jitter[2]", "jitter", 0 ],
			"obj-3::obj-184" : [ "wobble[2]", "wobble", 0 ],
			"obj-3::obj-185" : [ "warble[2]", "warble", 0 ],
			"obj-3::obj-33" : [ "mixitup[5]", "mixitup", 0 ],
			"obj-3::obj-40" : [ "reverse[2]", "reverse", 0 ],
			"obj-4::obj-117" : [ "active[3]", "active", 0 ],
			"obj-4::obj-123" : [ "gain[3]", "gain", 0 ],
			"obj-4::obj-125" : [ "texture[3]", "texture", 0 ],
			"obj-4::obj-128" : [ "mixitup[6]", "mixitup", 0 ],
			"obj-4::obj-171" : [ "pan[3]", "pan", 0 ],
			"obj-4::obj-176" : [ "shape[3]", "shape", 0 ],
			"obj-4::obj-177" : [ "transp[3]", "transp", 0 ],
			"obj-4::obj-179" : [ "size[3]", "size", 0 ],
			"obj-4::obj-180" : [ "density[3]", "density", 0 ],
			"obj-4::obj-181" : [ "scatter[3]", "scatter", 0 ],
			"obj-4::obj-182" : [ "spray[3]", "spray", 0 ],
			"obj-4::obj-183" : [ "jitter[3]", "jitter", 0 ],
			"obj-4::obj-184" : [ "wobble[3]", "wobble", 0 ],
			"obj-4::obj-185" : [ "warble[3]", "warble", 0 ],
			"obj-4::obj-33" : [ "mixitup[7]", "mixitup", 0 ],
			"obj-4::obj-40" : [ "reverse[3]", "reverse", 0 ],
			"obj-6" : [ "live.button[1]", "live.button", 0 ],
			"parameterbanks" : 			{

			}
,
			"parameter_overrides" : 			{
				"obj-2::obj-117" : 				{
					"parameter_longname" : "active[1]"
				}
,
				"obj-2::obj-123" : 				{
					"parameter_longname" : "gain[1]"
				}
,
				"obj-2::obj-125" : 				{
					"parameter_longname" : "texture[1]"
				}
,
				"obj-2::obj-128" : 				{
					"parameter_longname" : "mixitup[3]"
				}
,
				"obj-2::obj-171" : 				{
					"parameter_longname" : "pan[1]"
				}
,
				"obj-2::obj-176" : 				{
					"parameter_longname" : "shape[1]"
				}
,
				"obj-2::obj-177" : 				{
					"parameter_longname" : "transp[1]"
				}
,
				"obj-2::obj-179" : 				{
					"parameter_longname" : "size[1]"
				}
,
				"obj-2::obj-180" : 				{
					"parameter_longname" : "density[1]"
				}
,
				"obj-2::obj-181" : 				{
					"parameter_longname" : "scatter[1]"
				}
,
				"obj-2::obj-182" : 				{
					"parameter_longname" : "spray[1]"
				}
,
				"obj-2::obj-183" : 				{
					"parameter_longname" : "jitter[1]"
				}
,
				"obj-2::obj-184" : 				{
					"parameter_longname" : "wobble[1]"
				}
,
				"obj-2::obj-185" : 				{
					"parameter_longname" : "warble[1]"
				}
,
				"obj-2::obj-33" : 				{
					"parameter_longname" : "mixitup[2]"
				}
,
				"obj-2::obj-40" : 				{
					"parameter_longname" : "reverse[1]"
				}
,
				"obj-3::obj-117" : 				{
					"parameter_longname" : "active[2]"
				}
,
				"obj-3::obj-123" : 				{
					"parameter_longname" : "gain[2]"
				}
,
				"obj-3::obj-125" : 				{
					"parameter_longname" : "texture[2]"
				}
,
				"obj-3::obj-128" : 				{
					"parameter_longname" : "mixitup[4]"
				}
,
				"obj-3::obj-171" : 				{
					"parameter_longname" : "pan[2]"
				}
,
				"obj-3::obj-176" : 				{
					"parameter_longname" : "shape[2]"
				}
,
				"obj-3::obj-177" : 				{
					"parameter_longname" : "transp[2]"
				}
,
				"obj-3::obj-179" : 				{
					"parameter_longname" : "size[2]"
				}
,
				"obj-3::obj-180" : 				{
					"parameter_longname" : "density[2]"
				}
,
				"obj-3::obj-181" : 				{
					"parameter_longname" : "scatter[2]"
				}
,
				"obj-3::obj-182" : 				{
					"parameter_longname" : "spray[2]"
				}
,
				"obj-3::obj-183" : 				{
					"parameter_longname" : "jitter[2]"
				}
,
				"obj-3::obj-184" : 				{
					"parameter_longname" : "wobble[2]"
				}
,
				"obj-3::obj-185" : 				{
					"parameter_longname" : "warble[2]"
				}
,
				"obj-3::obj-33" : 				{
					"parameter_longname" : "mixitup[5]"
				}
,
				"obj-3::obj-40" : 				{
					"parameter_longname" : "reverse[2]"
				}
,
				"obj-4::obj-117" : 				{
					"parameter_longname" : "active[3]"
				}
,
				"obj-4::obj-123" : 				{
					"parameter_longname" : "gain[3]"
				}
,
				"obj-4::obj-125" : 				{
					"parameter_longname" : "texture[3]"
				}
,
				"obj-4::obj-128" : 				{
					"parameter_longname" : "mixitup[6]"
				}
,
				"obj-4::obj-171" : 				{
					"parameter_longname" : "pan[3]"
				}
,
				"obj-4::obj-176" : 				{
					"parameter_longname" : "shape[3]"
				}
,
				"obj-4::obj-177" : 				{
					"parameter_longname" : "transp[3]"
				}
,
				"obj-4::obj-179" : 				{
					"parameter_longname" : "size[3]"
				}
,
				"obj-4::obj-180" : 				{
					"parameter_longname" : "density[3]"
				}
,
				"obj-4::obj-181" : 				{
					"parameter_longname" : "scatter[3]"
				}
,
				"obj-4::obj-182" : 				{
					"parameter_longname" : "spray[3]"
				}
,
				"obj-4::obj-183" : 				{
					"parameter_longname" : "jitter[3]"
				}
,
				"obj-4::obj-184" : 				{
					"parameter_longname" : "wobble[3]"
				}
,
				"obj-4::obj-185" : 				{
					"parameter_longname" : "warble[3]"
				}
,
				"obj-4::obj-33" : 				{
					"parameter_longname" : "mixitup[7]"
				}
,
				"obj-4::obj-40" : 				{
					"parameter_longname" : "reverse[3]"
				}

			}
,
			"inherited_shortname" : 1
		}
,
		"dependency_cache" : [ 			{
				"name" : "lockstate.js",
				"bootpath" : "~/Desktop/huygens/max",
				"patcherrelativepath" : ".",
				"type" : "TEXT",
				"implicit" : 1
			}
, 			{
				"name" : "o.accum.maxpat",
				"bootpath" : "~/Documents/Max 8/Packages/odot/patchers/namespace",
				"patcherrelativepath" : "../../../Documents/Max 8/Packages/odot/patchers/namespace",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "o.compose.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.display.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.pack.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.prepend.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "o.union.mxo",
				"type" : "iLaX"
			}
, 			{
				"name" : "osc_granny.maxpat",
				"bootpath" : "~/Desktop/huygens/max",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
, 			{
				"name" : "transratio.maxpat",
				"bootpath" : "~/Library/Application Support/Cycling '74/Max 8/Examples/max-tricks/notes-and-pitch/pitch-to-freq-ratio",
				"patcherrelativepath" : "../../../Library/Application Support/Cycling '74/Max 8/Examples/max-tricks/notes-and-pitch/pitch-to-freq-ratio",
				"type" : "JSON",
				"implicit" : 1
			}
 ],
		"autosave" : 0
	}

}
