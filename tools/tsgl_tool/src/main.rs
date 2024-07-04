use nfd::Response;
use std::path::*;

mod ui;
mod font;

fn main() {
    let result = nfd::open_file_dialog(None, None).unwrap();

    match result {
        Response::Okay(path) => {
            font::process_font(&Path::new(&path), 50, 80.0);
        },
        Response::Cancel => println!("User canceled"),
        _ => ()
    }

    ui::run();
}
