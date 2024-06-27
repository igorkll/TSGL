use fontdue;
use nfd::Response;

use std::fs;
use std::ops::Add;
use std::path::*;
use std::u8;

mod ui;

fn gen_ascii(start: char, end: char) -> String {
    let mut result = String::new();
    for c in start..=end {
        result.push(c);
    }
    return result;
}

fn parse(path: &Path, px: f32, doorstep:u8, charmaps: &Vec<String>) -> Vec<u8> {
    let font = fs::read(path).unwrap();
    let font = font.as_slice();
    let font = fontdue::Font::from_bytes(font, fontdue::FontSettings::default()).unwrap();

    let mut out = Vec::new();

    for charmap in charmaps {
        for (i, c) in charmap.chars().enumerate() {
            let (metrics, bitmap) = font.rasterize(c, px);
            out.push(c as u8);
            let fwidth: u8 = (metrics.width - 1) as u8;
            let fheight: u8 = (metrics.height - 1) as u8;
            out.push(fwidth);
            out.push(fheight);
            if (doorstep )
        }
    }

    return out;
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
