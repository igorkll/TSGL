use fontdue;
use nfd::Response;

use std::fs;
use std::path::*;
use std::collections::HashMap;

mod ui;

struct TSGLChar {
    width: usize,
    height: usize,
    bitmap: Vec<u8>,
}

struct LoadedFont {
    positionInfo: Vec<usize>,
    HashMap: HashMap<char, Vec<TSGLChar>>,
}

fn gen_ascii(start: char, end: char) -> String {
    let mut result = String::new();
    for c in start..=end {
        result.push(c);
    }
    return result;
}

fn parse(path: &Path, px: f32, charmaps: &Vec<String>) -> LoadedFont {
    let font = fs::read(path).unwrap();
    let font = font.as_slice();
    let font = fontdue::Font::from_bytes(font, fontdue::FontSettings::default()).unwrap();


    for charmap in charmaps {
        for (i, c) in charmap.chars().enumerate() {
            let (metrics, bitmap) = font.rasterize(c, px);

        }
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
            charmaps.push(String::from(gen_ascii('A', 'Z')));
            charmaps.push(String::from(gen_ascii('a', 'z')));
            charmaps.push(String::from(gen_ascii('0', '9')));
            parse(&Path::new(&path), &charmaps);
        },
        Response::Cancel => println!("User canceled"),
        _ => ()
    }

    ui::run();
}
