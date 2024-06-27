use fontdue;
use nfd::Response;

use std::fs;
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

    if doorstep == 255 {
        out.push(1 as u8);
    } else {
        out.push(0 as u8);
    }
    for charmap in charmaps {
        for (_i, c) in charmap.chars().enumerate() {
            let (metrics, bitmap) = font.rasterize(c, px);
            out.push(c as u8);
            out.push((metrics.width >> 8) as u8);
            out.push((metrics.width & 0xff) as u8);
            out.push((metrics.height >> 8) as u8);
            out.push((metrics.height & 0xff) as u8);
            
            if doorstep == 255 {
                for py in 0..metrics.height {
                    for px in 0..metrics.width {
                        out.push(bitmap[px + (py * metrics.width)]);
                    }
                }
            } else {
                let mut write_byte: u8 = 0;
                let mut byte_index: u8 = 0;
                for py in 0..metrics.height {
                    for px in 0..metrics.width {
                        if bitmap[px + (py * metrics.width)] > doorstep {
                            write_byte |= 1 << byte_index;
                        }
                        byte_index += 1;
                        if byte_index > 7 {
                            byte_index = 0;
                            out.push(write_byte);
                        }
                    }
                }
                if byte_index > 0 {
                    out.push(write_byte);
                }
            }
        }
    }
    out.push(0 as u8);

    return out;
}

fn generate_info(name: &String) -> String {
    let info = String::new();

    

    return info;
}

fn generate_header(name: String) -> String {
    let mut header = String::new();
    header.push_str(&generate_info(&name));
    header.push_str("#pragma once\n#include <stdint.h>\n\nextern const uint8_t ");
    header.push_str(&name);
    header.push_str("[];");
    return header;
}

fn generate_executer(data: &Vec<u8>, name: String) -> String {
    let mut header = String::new();
    header.push_str("#include <stdint.h>\n\nconst uint8_t ");
    header.push_str(&name);
    header.push_str("[] = {");
    let mut terminator_add = false;
    for num in data {
        if terminator_add {
            header.push_str(", ");
        }
        header.push_str(&num.to_string());
        terminator_add = true;
    }
    header.push_str("};\n");
    return header;
}

fn main() {
    let result = nfd::open_file_dialog(None, None).unwrap();

    match result {
        Response::Okay(path) => {
            let mut charmaps: Vec<String> = Vec::new();
            charmaps.push(String::from(gen_ascii('A', 'Z')));
            charmaps.push(String::from(gen_ascii('a', 'z')));
            charmaps.push(String::from(gen_ascii('0', '9')));
            let parsed_font = parse(&Path::new(&path), 25.0, 100, &charmaps);
            fs::write(path.clone() + ".out", &parsed_font).expect("failed to write");
            fs::write(path.clone() + ".h", generate_header(String::from("font"))).expect("failed to write");
            fs::write(path.clone() + ".c", generate_executer(&parsed_font, String::from("font"))).expect("failed to write");
        },
        Response::Cancel => println!("User canceled"),
        _ => ()
    }

    ui::run();
}
