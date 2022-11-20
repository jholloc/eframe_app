use egui::plot::{Line, PlotPoints};
use poll_promise::Promise;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::Debug;
use std::mem::size_of;

#[derive(Serialize, Deserialize, Debug)]
struct Data {
    #[serde(rename = "_dtype")]
    dtype: String,
    #[serde(rename = "_encoding")]
    encoding: String,
    value: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct Dim {
    #[serde(rename = "_type")]
    obj_type: String,
    data: Data,
    label: String,
    units: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct Meta {
    exp_number: i32,
    filename: String,
    format: String,
    pass: i32,
    pass_date: String,
    path: String,
    signal_alias: String,
    signal_name: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct Signal {
    #[serde(rename = "_type")]
    obj_type: String,
    data: Data,
    dims: Vec<Dim>,
    label: String,
    meta: Meta,
    units: String,
}

#[derive(Debug)]
struct SignalResource {
    _response: ehttp::Response,
    signal: Option<Signal>,
}

impl SignalResource {
    #[allow(dead_code)]
    fn from_response(_ctx: &egui::Context, response: ehttp::Response) -> Self {
        let text = response.text().unwrap();
        let signal = serde_json::from_str(text).ok();
        Self {
            _response: response,
            signal,
        }
    }
}

struct SignalsResource {
    _response: ehttp::Response,
    signals: Option<Value>,
}

impl SignalsResource {
    fn from_response(_ctx: &egui::Context, response: ehttp::Response) -> Self {
        let text = response.text().unwrap();
        let signals: Value = serde_json::from_str(text).unwrap();
        Self {
            _response: response,
            signals: Some(signals),
        }
    }
}

/// We derive Deserialize/Serialize so we can persist app state on shutdown.
#[derive(Deserialize, Serialize, Default)]
#[serde(default)] // if we add new fields, give them default values when deserializing old state
pub struct TemplateApp {
    #[serde(skip)]
    fetched: bool,

    #[serde(skip)]
    signals_promise: Option<Promise<ehttp::Result<SignalsResource>>>,

    #[serde(skip)]
    signal_promise: Option<Promise<ehttp::Result<SignalResource>>>,
}

impl TemplateApp {
    /// Called once before the first frame.
    pub fn new(cc: &eframe::CreationContext<'_>) -> Self {
        // This is also where you can customized the look at feel of egui using
        // `cc.egui_ctx.set_visuals` and `cc.egui_ctx.set_fonts`.

        // Load previous app state (if any).
        // Note that you must enable the `persistence` feature for this to work.
        if let Some(storage) = cc.storage {
            return eframe::get_value(storage, eframe::APP_KEY).unwrap_or_default();
        }

        Default::default()
    }
}

trait ByteDecode {
    fn size() -> usize
    where
        Self: Sized,
    {
        size_of::<Self>()
    }
    fn decode(bytes: &[u8]) -> Vec<Self>
    where
        Self: Sized,
    {
        assert_eq!(bytes.len() % Self::size(), 0);
        let mut output = vec![];
        for window in bytes.chunks(Self::size()) {
            output.push(Self::from_ne_bytes(window));
        }
        output
    }
    fn from_ne_bytes(bytes: &[u8]) -> Self;
}

impl ByteDecode for i16 {
    fn from_ne_bytes(bytes: &[u8]) -> Self {
        Self::from_ne_bytes(bytes.try_into().unwrap())
    }
}
impl ByteDecode for f32 {
    fn from_ne_bytes(bytes: &[u8]) -> Self {
        Self::from_ne_bytes(bytes.try_into().unwrap())
    }
}

fn plot<T, U>(ui: &mut egui::Ui, x_values: Vec<T>, y_values: Vec<U>)
where
    T: Debug + Sized + Into<f64> + Copy,
    U: Debug + Sized + Into<f64> + Copy,
{
    // ui.label(format!("{:?}", x_values));
    // ui.label(format!("{:?}", y_values));

    let line_points: PlotPoints = x_values
        .iter()
        .zip(y_values.iter())
        .map(|(&x, &y)| [x.into(), y.into()])
        .collect();
    let line = Line::new(line_points);

    egui::plot::Plot::new("signal_plot")
        .height(500.)
        .data_aspect(1.0)
        .view_aspect(1.0)
        .show(ui, |plot_ui| {
            plot_ui.line(line);
        });
}

fn show_plot(ui: &mut egui::Ui, signal: &Signal) {
    let rank = signal.dims.len();
    if rank == 1 {
        let dim = &signal.dims[0];
        let dim_data = &dim.data;
        let d_type = &dim_data.dtype;
        let bytes64 = &dim_data.value;
        if let Ok(dim_bytes) = base64::decode_config(bytes64, base64::URL_SAFE) {
            match &d_type[..] {
                "float32" => {
                    let data = &signal.data;
                    let d_type = &data.dtype;
                    let bytes64 = &data.value;
                    if let Ok(bytes) = base64::decode_config(bytes64, base64::URL_SAFE) {
                        match &d_type[..] {
                            "int16" => plot(ui, f32::decode(&dim_bytes), i16::decode(&bytes)),
                            "float32" => plot(ui, f32::decode(&dim_bytes), f32::decode(&bytes)),
                            _ => {
                                ui.label(format!("unknown data type {d_type}"));
                            }
                        }
                    }
                }
                _ => {
                    ui.label(format!("unknown data type {d_type}"));
                }
            }
        }
    } else {
        ui.label("Cannot plot data with rank != 1");
    }
}

fn show_meta(ui: &mut egui::Ui, signal: &Signal) {
    egui::Grid::new("meta-grid").show(ui, |ui| {
        ui.label("exp_number");
        ui.label(signal.meta.exp_number.to_string());
        ui.end_row();

        ui.label("filename");
        ui.label(signal.meta.filename.to_string());
        ui.end_row();

        ui.label("format");
        ui.label(signal.meta.format.to_string());
        ui.end_row();

        ui.label("pass");
        ui.label(signal.meta.pass.to_string());
        ui.end_row();

        ui.label("pass_date");
        ui.label(signal.meta.pass_date.to_string());
        ui.end_row();

        ui.label("path");
        ui.label(signal.meta.path.to_string());
        ui.end_row();

        ui.label("signal_alias");
        ui.label(signal.meta.signal_alias.to_string());
        ui.end_row();

        ui.label("signal_name");
        ui.label(signal.meta.signal_name.to_string());
        ui.end_row();
    });
}

impl eframe::App for TemplateApp {
    /// Called each time the UI needs repainting, which may be many times per second.
    /// Put your widgets into a `SidePanel`, `TopPanel`, `CentralPanel`, `Window` or `Area`.
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        #[cfg(not(target_arch = "wasm32"))] // no File->Quit on web pages!
        egui::TopBottomPanel::top("top_panel").show(ctx, |ui| {
            // The top panel is often a good place for a menu bar:
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Quit").clicked() {
                        _frame.close();
                    }
                });
            });
        });

        egui::SidePanel::left("side_panel").show(ctx, |ui| {
            ui.heading("Signals");
            ui.separator();

            egui::ScrollArea::vertical().show(ui, |ui| {
                let mut trigger_fetch_signals = false;
                let mut trigger_fetch_signal = false;
                let mut fetch_signal = "".to_owned();

                ui.add_visible_ui(!self.fetched, |ui| {
                    if ui.button("Fetch").clicked() {
                        trigger_fetch_signals = true;
                    }
                });

                if trigger_fetch_signals {
                    let ctx = ctx.clone();
                    let (sender, promise) = Promise::new();
                    let request = ehttp::Request::get("http://localhost:5000/signals");
                    ehttp::fetch(request, move |response| {
                        ctx.request_repaint();
                        let resource =
                            response.map(|response| SignalsResource::from_response(&ctx, response));
                        sender.send(resource);
                    });
                    self.signals_promise = Some(promise);
                }

                if let Some(promise) = &self.signals_promise {
                    if let Some(result) = promise.ready() {
                        match result {
                            Ok(resource) => {
                                self.fetched = true;
                                if let Some(signals) = &resource.signals {
                                    if let Some(map) = signals.as_object() {
                                        for item in map.iter() {
                                            let alias = item.0;
                                            ui.collapsing(alias, |ui| {
                                                let signals = item.1.as_array().unwrap();
                                                for value in signals {
                                                    let signal = value.as_str().unwrap();
                                                    if ui.button(signal).clicked() {
                                                        let signal = format!("{alias}_{signal}");
                                                        trigger_fetch_signal = true;
                                                        fetch_signal = signal;
                                                    }
                                                }
                                            });
                                        }
                                    }
                                }
                            }
                            Err(error) => {
                                ui.colored_label(
                                    ui.visuals().error_fg_color,
                                    if error.is_empty() { "Error" } else { error },
                                );
                            }
                        }
                    }
                }

                if trigger_fetch_signal {
                    let ctx = ctx.clone();
                    let (sender, promise) = Promise::new();
                    let url = format!("http://localhost:5000/signal/{fetch_signal}");
                    let request = ehttp::Request::get(url);
                    ehttp::fetch(request, move |response| {
                        ctx.request_repaint();
                        let resource =
                            response.map(|response| SignalResource::from_response(&ctx, response));
                        sender.send(resource);
                    });
                    self.signal_promise = Some(promise);
                }

                ui.with_layout(egui::Layout::bottom_up(egui::Align::LEFT), |ui| {
                    ui.horizontal(|ui| {
                        ui.spacing_mut().item_spacing.x = 0.0;
                        ui.label("powered by ");
                        ui.hyperlink_to("egui", "https://github.com/emilk/egui");
                        ui.label(" and ");
                        ui.hyperlink_to(
                            "eframe",
                            "https://github.com/emilk/egui/tree/master/crates/eframe",
                        );
                        ui.label(".");
                    });
                });
            });
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            // The central panel the region left after adding TopPanel's and SidePanel's

            ui.heading("Plot");
            ui.separator();

            if let Some(promise) = &self.signal_promise {
                if let Some(result) = promise.ready() {
                    match result {
                        Ok(resource) => {
                            if let Some(signal) = &resource.signal {
                                show_meta(ui, signal);
                                ui.separator();
                                show_plot(ui, signal);
                            }
                        }
                        Err(error) => {
                            ui.colored_label(
                                ui.visuals().error_fg_color,
                                if error.is_empty() { "Error" } else { error },
                            );
                        }
                    }
                }
            }
        });

        // if false {
        //     egui::Window::new("Window").show(ctx, |ui| {
        //         ui.label("Windows can be moved by dragging them.");
        //         ui.label("They are automatically sized based on contents.");
        //         ui.label("You can turn on resizing and scrolling if you like.");
        //         ui.label("You would normally chose either panels OR windows.");
        //     });
        // }
    }

    /// Called by the frame work to save state before shutdown.
    fn save(&mut self, storage: &mut dyn eframe::Storage) {
        eframe::set_value(storage, eframe::APP_KEY, self);
    }
}
