use fontdue;
use nfd::Response;

use std::fs;
use std::path::*;

mod ui;

struct LoadedFont {
    width: usize,
    height: usize,
    bitmap: Vec<u8>,
}

fn parse(path: &Path, charmaps: &Vec<String>) -> LoadedFont {
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

    for charmap in charmaps {
        println!("{}", charmap);
    }

    return LoadedFont {
        width: metrics.width,
        height: metrics.height,
        bitmap: bitmap
    };
}

fn main() {
    let result = nfd::open_file_dialog(None, None).unwrap();

    match result {
        Response::Okay(path) => {
            let mut charmaps: Vec<String> = Vec::new();
            charmaps.push(String::from("Привет"));
            parse(&Path::new(&path), &charmaps);
        },
        Response::Cancel => println!("User canceled"),
        _ => ()
    }

    ui::run();
}
