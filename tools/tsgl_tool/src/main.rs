use fontdue;
use nfd::Response;

use std::fs;
use std::path::*;
use std::u8;
use std::string::String;

mod ui;

fn gen_ascii(start: char, end: char) -> String {
    let mut result = String::new();
    for c in start..=end {
        result.push(c);
    }
    return result;
}

fn parse(path: &Path, px: f32, contrast:u8, charmaps: &Vec<String>) -> Vec<u8> {
    let font = fs::read(path).unwrap();
    let font = font.as_slice();
    let font = fontdue::Font::from_bytes(font, fontdue::FontSettings::default()).unwrap();

    let mut out = Vec::new();

    if contrast == 255 {
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
            
            if contrast == 255 {
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
                        if bitmap[px + (py * metrics.width)] > contrast {
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

fn generate_info(name: &String, px: f32, contrast:u8) -> String {
    let mut info = String::new();
    info.push_str("// TSGL font\n// name - ");
    info.push_str(name);
    info.push_str("\n// size - ");
    info.push_str(&px.to_string());
    if contrast == 255 {
        info.push_str("\n// anti-aliasing is enabled");
    } else {
        info.push_str("\n// contrast - ");
        info.push_str(&contrast.to_string());
    }
    return info;
}

fn generate_header(name: &String, info: &String) -> String {
    let mut header = String::new();
    header.push_str(info);
    header.push_str("\n\n");
    header.push_str("#pragma once\n#include <stdint.h>\n\nextern const uint8_t ");
    header.push_str(&name);
    header.push_str("[];");
    return header;
}

fn generate_executable(data: &Vec<u8>, name: &String, info: &String) -> String {
    let mut executable = String::new();
    executable.push_str(info);
    executable.push_str("\n\n");
    executable.push_str("#include <stdint.h>\n\nconst uint8_t ");
    executable.push_str(&name);
    executable.push_str("[] = {");
    let mut terminator_add = false;
    for num in data {
        if terminator_add {
            executable.push_str(", ");
        }
        executable.push_str(&num.to_string());
        terminator_add = true;
    }
    executable.push_str("};\n");
    return executable;
}

fn process_font(path: &String) {
    let path = Path::new(path);
    let contrast = 100;
    let px =25.0;
    let name = path.with_extension("").file_name().unwrap().to_str().unwrap().to_string();
    let info = generate_info(&name, px, contrast);

    let mut charmaps: Vec<String> = Vec::new();
    charmaps.push(String::from(gen_ascii('A', 'Z')));
    charmaps.push(String::from(gen_ascii('a', 'z')));
    charmaps.push(String::from(gen_ascii('0', '9')));

    let parsed_font = parse(&path, px, contrast, &charmaps);
    fs::write(path.with_extension("tgf"), &parsed_font).expect("failed to write");
    fs::write(path.with_extension("h"), generate_header(&name, &info)).expect("failed to write");
    fs::write(path.with_extension("c"), generate_executable(&parsed_font, &String::from("font"), &info)).expect("failed to write");
}

fn main() {
    let result = nfd::open_file_dialog(None, None).unwrap();

    match result {
        Response::Okay(path) => {
            process_font(&path);
        },
        Response::Cancel => println!("User canceled"),
        _ => ()
    }

    ui::run();
}
