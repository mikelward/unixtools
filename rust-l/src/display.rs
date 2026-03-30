const OUTER_MARGIN: usize = 2;

fn ceil_div(num: usize, div: usize) -> usize {
    if div == 0 {
        return num;
    }
    (num + div - 1) / div
}

/// Print items in columns, filling top-to-bottom then left-to-right
pub fn print_down(items: &[String], item_width: usize, screen_width: usize) {
    let n = items.len();
    if n == 0 {
        return;
    }

    let col_width = item_width + OUTER_MARGIN;
    if col_width == 0 {
        for item in items {
            println!("{}", item);
        }
        return;
    }

    let max_cols = std::cmp::max(1, screen_width / col_width);
    let rows = ceil_div(n, max_cols);
    let cols = ceil_div(n, rows);

    for row in 0..rows {
        for col in 0..cols {
            let idx = col * rows + row;
            if idx >= n {
                break;
            }
            if col > 0 {
                print!("{}", " ".repeat(OUTER_MARGIN));
            }
            print!("{}", items[idx]);
            // Pad to item_width if not last column
            if col < cols - 1 && col * rows + row + rows < n {
                let visible = visible_width(&items[idx]);
                if visible < item_width {
                    print!("{}", " ".repeat(item_width - visible));
                }
            }
        }
        println!();
    }
}

/// Print items in rows, filling left-to-right then top-to-bottom
pub fn print_across(items: &[String], item_width: usize, screen_width: usize) {
    let n = items.len();
    if n == 0 {
        return;
    }

    let col_width = item_width + OUTER_MARGIN;
    let cols = if col_width > 0 {
        std::cmp::max(1, screen_width / col_width)
    } else {
        1
    };

    for (i, item) in items.iter().enumerate() {
        if i > 0 && i % cols == 0 {
            println!();
        } else if i > 0 {
            print!("{}", " ".repeat(OUTER_MARGIN));
        }
        print!("{}", item);
        let col_pos = i % cols;
        if col_pos < cols - 1 && i + 1 < n {
            let visible = visible_width(item);
            if visible < item_width {
                print!("{}", " ".repeat(item_width - visible));
            }
        }
    }
    println!();
}

/// Calculate visible width (excluding ANSI escape sequences)
fn visible_width(s: &str) -> usize {
    let mut width = 0;
    let mut in_escape = false;
    for c in s.chars() {
        if in_escape {
            if c.is_ascii_alphabetic() {
                in_escape = false;
            }
        } else if c == '\x1b' {
            in_escape = true;
        } else {
            width += unicode_width::UnicodeWidthChar::width(c).unwrap_or(0);
        }
    }
    width
}
