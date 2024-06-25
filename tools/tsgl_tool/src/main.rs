use fontdue;
use image;

fn main() {
    let font = include_bytes!("font.ttf") as &[u8];
    let font = fontdue::Font::from_bytes(font, fontdue::FontSettings::default()).unwrap();
    let (metrics, bitmap) = font.rasterize('b', 50.0);

    for py in 0..metrics.height {
        for px in 0..metrics.width {
            if bitmap[px + (py * metrics.width)] > 100 {
                print!("██");
            } else {
                print!("  ");
            }
        }
        println!("");
    }
}
