extern crate native_windows_gui as nwg;
use nwg::NativeUi;
use nwg::Event as Event;

use std::rc::*;
use std::cell::*;
use std::ops::*;

#[derive(Default)]
pub struct TSGLTool {
    window: nwg::Window,
    load_font: nwg::Button
}

impl TSGLTool {
    fn say_hello(&self) {
        //nwg::modal_info_message(&self.window, "Hello", &format!("Hello {}", self.name_edit.text()));
    }
    
    fn say_goodbye(&self) {
        //nwg::modal_info_message(&self.window, "Goodbye", &format!("Goodbye {}", self.name_edit.text()));
        //nwg::stop_thread_dispatch();
    }
}

pub struct TSGLToolUi {
    inner: Rc<TSGLTool>,
    default_handler: RefCell<Option<nwg::EventHandler>>
}

impl nwg::NativeUi<TSGLToolUi> for TSGLTool {
    fn build_ui(mut data: TSGLTool) -> Result<TSGLToolUi, nwg::NwgError> {
        // Controls
        nwg::Window::builder()
            .flags(nwg::WindowFlags::WINDOW | nwg::WindowFlags::VISIBLE)
            .size((500, 135))
            .title("TSGL Tool")
            .build(&mut data.window)?;

        nwg::Button::builder()
            .size((280, 70))
            .position((10, 50))
            .text("convert font")
            .parent(&data.window)
            .build(&mut data.load_font)?;

        // Wrap-up
        let ui = TSGLToolUi {
            inner: Rc::new(data),
            default_handler: Default::default(),
        };

        // Events
        let evt_ui = Rc::downgrade(&ui.inner);
        let handle_events = move |evt, _evt_data, handle| {
            if let Some(ui) = evt_ui.upgrade() {
                match evt {
                    Event::OnButtonClick => 
                        if &handle == &ui.load_font {
                            TSGLTool::say_hello(&ui);
                        },
                    Event::OnWindowClose => 
                        if &handle == &ui.window {
                            TSGLTool::say_goodbye(&ui);
                        },
                    _ => {}
                }
            }
        };

        *ui.default_handler.borrow_mut() = Some(nwg::full_bind_event_handler(&ui.window.handle, handle_events));

        return Ok(ui);
    }
}

impl Drop for TSGLToolUi {
    fn drop(&mut self) {
        let handler = self.default_handler.borrow();
        if handler.is_some() {
            nwg::unbind_event_handler(handler.as_ref().unwrap());
        }
    }
}

impl Deref for TSGLToolUi {
    type Target = TSGLTool;

    fn deref(&self) -> &TSGLTool {
        &self.inner
    }
}

pub fn run() {
    nwg::init().expect("Failed to init Native Windows GUI");
    nwg::Font::set_global_family("Segoe UI").expect("Failed to set default font");
    let _ui = TSGLTool::build_ui(Default::default()).expect("Failed to build UI");
    nwg::dispatch_thread_events();
}