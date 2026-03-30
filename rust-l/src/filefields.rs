use crate::color::Colors;
use crate::field::{Align, Field};
use crate::file::File;
use crate::options::*;

pub fn get_file_fields(file: &File, options: &Options) -> Vec<Field> {
    let mut fields = Vec::new();

    // If target_info and file is a symlink, we show info about the target
    // but display the original name
    let target_stat = if options.target_info && file.is_link() {
        file.get_final_target_stat()
    } else {
        None
    };

    if options.size {
        fields.push(get_size_field(file, options));
    }
    if options.inode {
        fields.push(get_inode_field(file));
    }
    if options.modes {
        fields.push(get_modes_field(file));
    }
    if options.link_count {
        fields.push(get_link_count_field(file));
    }
    if options.owner {
        fields.push(get_owner_field(file, options));
    }
    if options.group {
        fields.push(get_group_field(file, options));
    }
    if options.perms {
        fields.push(get_perms_field(file));
    }
    if options.bytes {
        fields.push(get_bytes_field(file, options, &target_stat));
    }
    if options.datetime {
        fields.push(get_datetime_field(file, options, &target_stat));
    }

    fields.push(get_name_field(file, options));

    fields
}

fn get_size_field(file: &File, options: &Options) -> Field {
    let blocks = file.get_blocks(options.blocksize);
    Field::right(format!("{}", blocks))
}

fn get_inode_field(file: &File) -> Field {
    Field::right(format!("{}", file.get_inode()))
}

fn get_modes_field(file: &File) -> Field {
    Field::left(file.get_modes())
}

fn get_link_count_field(file: &File) -> Field {
    Field::right(format!("{}", file.get_nlink()))
}

fn get_owner_field(file: &File, options: &Options) -> Field {
    let uid = file.get_uid();
    let name = if options.numeric {
        format!("{}", uid)
    } else {
        get_username(uid).unwrap_or_else(|| format!("{}", uid))
    };
    Field::left(name)
}

fn get_group_field(file: &File, options: &Options) -> Field {
    let gid = file.get_gid();
    let name = if options.numeric {
        format!("{}", gid)
    } else {
        get_groupname(gid).unwrap_or_else(|| format!("{}", gid))
    };
    Field::left(name)
}

fn get_perms_field(file: &File) -> Field {
    Field::right(file.get_perms())
}

fn get_bytes_field(file: &File, options: &Options, target_stat: &Option<libc::stat>) -> Field {
    let size = if let Some(st) = target_stat {
        st.st_size
    } else {
        file.get_size()
    };

    let s = match options.size_style {
        SizeStyle::Human => human_bytes(size as u64),
        SizeStyle::Default => format!("{}", size),
    };
    Field::right(s)
}

fn get_datetime_field(file: &File, options: &Options, target_stat: &Option<libc::stat>) -> Field {
    let time = if let Some(st) = target_stat {
        match options.time_type {
            TimeType::Mtime => st.st_mtime,
            TimeType::Atime => st.st_atime,
            TimeType::Ctime => st.st_ctime,
        }
    } else {
        match options.time_type {
            TimeType::Mtime => file.get_mtime(),
            TimeType::Atime => file.get_atime(),
            TimeType::Ctime => file.get_ctime(),
        }
    };

    let s = match options.time_style {
        TimeStyle::Iso => format_time_iso(time),
        TimeStyle::Relative => format_time_relative(time, options.now),
        TimeStyle::Traditional => format_time_traditional(time, options.now),
    };
    Field::right(s)
}

fn get_name_field(file: &File, options: &Options) -> Field {
    let mut buf = String::new();
    let colors = if options.color { Colors::new() } else { None };

    // Opening flag for old-style
    if options.flags == FlagStyle::Old {
        if file.is_dir() {
            buf.push('[');
        } else if file.is_exec() {
            buf.push('*');
        }
    }

    // Color start
    if let Some(ref c) = colors {
        if file.is_dir() {
            buf.push_str(&c.blue);
        } else if file.is_link() {
            buf.push_str(&c.cyan);
        } else if file.is_exec() {
            buf.push_str(&c.green);
        }
    }

    // The name itself, with escaping applied
    let name = escape_name(file.name(), options.escape);
    buf.push_str(&name);

    // Color end
    if let Some(ref c) = colors {
        if file.is_dir() || file.is_link() || file.is_exec() {
            buf.push_str(&c.none);
        }
    }

    // Closing flag for old-style
    if options.flags == FlagStyle::Old {
        if file.is_dir() {
            buf.push(']');
        } else if file.is_exec() {
            buf.push('*');
        }
    }

    // Normal flags (-F)
    if options.flags == FlagStyle::Normal {
        if file.is_dir() {
            buf.push('/');
        } else if file.is_link() {
            buf.push('@');
        } else if file.is_exec() {
            buf.push('*');
        } else if file.is_fifo() {
            buf.push('|');
        } else if file.is_sock() {
            buf.push('=');
        }
    }

    // Symlink display
    if options.show_links && file.is_link() {
        let chain = file.get_link_chain();
        for target_name in &chain {
            buf.push_str(" -> ");
            buf.push_str(&escape_name(target_name, options.escape));
        }
    } else if options.show_link && file.is_link() {
        if let Some(target) = file.get_target() {
            buf.push_str(" -> ");
            buf.push_str(&escape_name(target.name(), options.escape));
        }
    }

    // Calculate visible width (excluding escape sequences)
    let width = visible_width(&buf);
    Field::new(buf, Align::Left, width)
}

fn escape_name(name: &str, escape: EscapeMode) -> String {
    match escape {
        EscapeMode::None => name.to_string(),
        EscapeMode::Question => {
            name.chars()
                .map(|c| {
                    if c.is_control() {
                        '?'
                    } else {
                        c
                    }
                })
                .collect()
        }
        EscapeMode::C => {
            let mut s = String::new();
            for c in name.chars() {
                match c {
                    '\\' => s.push_str("\\\\"),
                    '\n' => s.push_str("\\n"),
                    '\t' => s.push_str("\\t"),
                    '\r' => s.push_str("\\r"),
                    '\x07' => s.push_str("\\a"),
                    '\x08' => s.push_str("\\b"),
                    '\x0c' => s.push_str("\\f"),
                    '\x0b' => s.push_str("\\v"),
                    '\x1b' => s.push_str("\\033"),
                    c if c.is_control() => {
                        s.push_str(&format!("\\{:03o}", c as u32));
                    }
                    c => s.push(c),
                }
            }
            s
        }
    }
}

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

fn human_bytes(bytes: u64) -> String {
    const UNITS: &[&str] = &["B", "KB", "MB", "GB", "TB", "PB", "EB"];
    let mut size = bytes as f64;
    for unit in UNITS {
        if size < 1024.0 {
            return format!("{:.0}{}", size, unit);
        }
        size /= 1024.0;
    }
    format!("{:.0}ZB", size)
}

fn format_time_traditional(time: i64, now: i64) -> String {
    use std::fmt::Write;
    let tm = unsafe {
        let t = time as libc::time_t;
        let mut tm: libc::tm = std::mem::zeroed();
        libc::localtime_r(&t, &mut tm);
        tm
    };

    let months = [
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    ];
    let month = months[tm.tm_mon as usize % 12];
    let day = tm.tm_mday;

    // If within ~6 months, show time; otherwise show year
    let six_months = 180 * 24 * 3600;
    let mut s = String::new();
    if (now - time).abs() < six_months {
        let _ = write!(s, "{} {:>2} {:02}:{:02}", month, day, tm.tm_hour, tm.tm_min);
    } else {
        let _ = write!(s, "{} {:>2}  {}", month, day, tm.tm_year + 1900);
    }
    s
}

fn format_time_iso(time: i64) -> String {
    let tm = unsafe {
        let t = time as libc::time_t;
        let mut tm: libc::tm = std::mem::zeroed();
        libc::localtime_r(&t, &mut tm);
        tm
    };
    format!(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec
    )
}

fn format_time_relative(time: i64, now: i64) -> String {
    let diff = now - time;
    if diff < 0 {
        return "future".to_string();
    }

    let seconds = diff;
    let minutes = seconds / 60;
    let hours = minutes / 60;
    let days = hours / 24;
    let months = days / 30;
    let years = days / 365;

    if years > 0 {
        format!("{} year{}", years, if years == 1 { "" } else { "s" })
    } else if months > 0 {
        format!("{} month{}", months, if months == 1 { "" } else { "s" })
    } else if days > 0 {
        format!("{} day{}", days, if days == 1 { "" } else { "s" })
    } else if hours > 0 {
        format!("{} hour{}", hours, if hours == 1 { "" } else { "s" })
    } else if minutes > 0 {
        format!("{} minute{}", minutes, if minutes == 1 { "" } else { "s" })
    } else {
        format!(
            "{} second{}",
            seconds,
            if seconds == 1 { "" } else { "s" }
        )
    }
}

fn get_username(uid: u32) -> Option<String> {
    unsafe {
        let pw = libc::getpwuid(uid);
        if pw.is_null() {
            None
        } else {
            let name = std::ffi::CStr::from_ptr((*pw).pw_name);
            Some(name.to_string_lossy().to_string())
        }
    }
}

fn get_groupname(gid: u32) -> Option<String> {
    unsafe {
        let gr = libc::getgrgid(gid);
        if gr.is_null() {
            None
        } else {
            let name = std::ffi::CStr::from_ptr((*gr).gr_name);
            Some(name.to_string_lossy().to_string())
        }
    }
}
