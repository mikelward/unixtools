#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Align {
    Left,
    Right,
}

#[derive(Debug, Clone)]
pub struct Field {
    pub string: String,
    pub align: Align,
    pub width: usize,
}

impl Field {
    pub fn new(string: String, align: Align, width: usize) -> Self {
        Field {
            string,
            align,
            width,
        }
    }

    pub fn left(string: String) -> Self {
        let width = unicode_width::UnicodeWidthStr::width(string.as_str());
        Field {
            string,
            align: Align::Left,
            width,
        }
    }

    pub fn right(string: String) -> Self {
        let width = unicode_width::UnicodeWidthStr::width(string.as_str());
        Field {
            string,
            align: Align::Right,
            width,
        }
    }
}
