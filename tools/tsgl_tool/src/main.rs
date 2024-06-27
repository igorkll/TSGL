use fontdue;
use nfd::Response;

use std::fs;
use std::path::*;

mod ui;

fn parse(path: &Path) {
    let font = fs::read(path).unwrap();
    let font = font.as_slice();
    let font = fontdue::Font::from_bytes(font, fontdue::FontSettings::default()).unwrap();
    let (metrics, bitmap) = font.rasterize('K', 25.0);

    for py in 0..metrics.height {
        for px in 0..metrics.width {
            if bitmap[px + (py * metrics.width)] > 25 {
                print!("██");
            } else {
                print!("  ");
            }
        }
        println!("");
    }
}

fn main() {
    let result = nfd::open_file_dialog(None, None).unwrap();

    match result {
        Response::Okay(path) => parse(&Path::new(&path)),
        Response::Cancel => println!("User canceled"),
        _ => ()
    }

    ui::run();
}
