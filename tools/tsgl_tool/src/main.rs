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
            out.push((metrics.width >> 8) as u8);
            out.push((metrics.width & 0xff) as u8);
            out.push((metrics.height >> 8) as u8);
            out.push((metrics.height & 0xff) as u8);
            
            if (doorstep == 255) {
                for py in 0..metrics.height {
                    for px in 0..metrics.width {
                        out.push(bitmap[px + (py * metrics.width)])
                    }
                }
            } else {
                for py in 0..metrics.height {
                    for px in 0..metrics.width {
                        if bitmap[px + (py * metrics.width)] > doorstep {
                            print!("██");
                        } else {
                            print!("  ");
                        }
                    }
                }    
            }
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
