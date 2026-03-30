//! l - a cleaner reimplementation of ls

mod color;
mod display;
mod field;
mod file;
mod filefields;
mod options;

use crate::display::{print_across, print_down};
use crate::file::File;
use crate::filefields::get_file_fields;
use crate::options::{DisplayMode, Options};

fn main() {
    let mut options = Options::new();
    let args: Vec<String> = std::env::args().collect();
    let file_args = options.parse_args(&args);

    let paths: Vec<&str> = if file_args.is_empty() {
        vec!["."]
    } else {
        file_args.iter().map(|s| s.as_str()).collect()
    };

    let mut files = Vec::new();
    let mut dirs = Vec::new();

    for path in &paths {
        let file = File::new("", path);
        if file.is_stat() {
            if !options.directory
                && (file.is_dir()
                    || (options.follow_dir_link_args && file.is_link_to_dir()))
            {
                dirs.push(file);
            } else {
                files.push(file);
            }
        } else {
            // file doesn't exist or can't be statted - error already printed
        }
    }

    let nfiles = files.len();
    list_files(&files, &options);

    let first_output = nfiles == 0;
    list_dirs(&dirs, &options, first_output);
}

fn make_file_strings(
    file_fields: &[Vec<crate::field::Field>],
    field_widths: &[usize],
) -> Vec<String> {
    let mut strings = Vec::new();
    let column_margin = 1;

    for fields in file_fields {
        let mut buf = String::new();
        let nfields = fields.len();
        for (j, field) in fields.iter().enumerate() {
            let padded_width = field_widths[j];
            let screen_width = field.width;

            if field.align == crate::field::Align::Right {
                for _ in 0..(padded_width.saturating_sub(screen_width)) {
                    buf.push(' ');
                }
            }
            buf.push_str(&field.string);
            if field.align == crate::field::Align::Left {
                for _ in 0..(padded_width.saturating_sub(screen_width)) {
                    buf.push(' ');
                }
            }
            if j != nfields - 1 {
                for _ in 0..column_margin {
                    buf.push(' ');
                }
            }
        }
        strings.push(buf);
    }
    strings
}

fn get_max_field_widths(file_fields: &[Vec<crate::field::Field>]) -> Vec<usize> {
    if file_fields.is_empty() {
        return Vec::new();
    }
    let nfields = file_fields[0].len();
    let mut widths = vec![0usize; nfields];
    for fields in file_fields {
        for (j, field) in fields.iter().enumerate() {
            if j < nfields && field.width > widths[j] {
                widths[j] = field.width;
            }
        }
    }
    widths
}

fn get_file_width(field_widths: &[usize]) -> usize {
    if field_widths.is_empty() {
        return 0;
    }
    let column_margin = 1;
    field_widths.iter().sum::<usize>() + column_margin * (field_widths.len() - 1)
}

fn list_files(files: &[File], options: &Options) {
    if files.is_empty() {
        return;
    }

    let mut sorted: Vec<&File> = files.iter().collect();
    sort_files(&mut sorted, options);

    if options.reverse {
        sorted.reverse();
    }

    let file_fields: Vec<Vec<crate::field::Field>> = sorted
        .iter()
        .map(|f| get_file_fields(f, options))
        .collect();

    let field_widths = get_max_field_widths(&file_fields);
    let file_strings = make_file_strings(&file_fields, &field_widths);
    let file_width = get_file_width(&field_widths);

    match options.display_mode {
        DisplayMode::OnePerLine => {
            for s in &file_strings {
                println!("{}", s);
            }
        }
        DisplayMode::InColumns => {
            print_down(&file_strings, file_width, options.screen_width as usize);
        }
        DisplayMode::InRows => {
            print_across(&file_strings, file_width, options.screen_width as usize);
        }
    }
}

fn list_dir(dir: &File, options: &Options) {
    let path = dir.path();
    let entries = match std::fs::read_dir(path) {
        Ok(e) => e,
        Err(e) => {
            eprintln!("l: Cannot open {}: {}", path, e);
            return;
        }
    };

    let mut files = Vec::new();
    let mut subdirs = Vec::new();
    let mut total_blocks: u64 = 0;

    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().to_string();
        if !options.all && name.starts_with('.') {
            continue;
        }
        let file = File::new(path, &name);
        if options.dirs_only && !file.is_dir() {
            continue;
        }
        if options.dir_totals {
            total_blocks += file.get_blocks(options.blocksize);
        }
        if options.recursive && file.is_dir() {
            subdirs.push(File::new(path, &name));
        }
        files.push(file);
    }

    if options.dir_totals {
        println!("total {}", total_blocks);
    }
    list_files(&files, options);
    if options.recursive {
        list_dirs(&subdirs, options, false);
    }
}

fn list_dirs(dirs: &[File], options: &Options, first_output: bool) {
    let ndirs = dirs.len();
    let need_label = ndirs > 1 || !first_output || options.recursive;
    let mut is_first = first_output;

    for (i, dir) in dirs.iter().enumerate() {
        if i > 0 {
            is_first = false;
        }
        if !is_first {
            println!();
        }
        if need_label {
            println!("{}:", dir.path());
        }
        list_dir(dir, options);
    }
}

fn sort_files(files: &mut Vec<&File>, options: &Options) {
    use crate::options::SortType;
    match options.sort_type {
        SortType::Unsorted => {}
        SortType::ByName => {
            files.sort_by(|a, b| a.name().cmp(b.name()));
        }
        SortType::BySize => {
            files.sort_by(|a, b| b.get_size().cmp(&a.get_size()));
        }
        SortType::ByTime => {
            let time_fn = match options.time_type {
                crate::options::TimeType::Atime => File::get_atime,
                crate::options::TimeType::Ctime => File::get_ctime,
                crate::options::TimeType::Mtime => File::get_mtime,
            };
            files.sort_by(|a, b| {
                let ta = time_fn(a);
                let tb = time_fn(b);
                tb.cmp(&ta).then_with(|| a.name().cmp(b.name()))
            });
        }
        SortType::ByVersion => {
            files.sort_by(|a, b| version_cmp(a.name(), b.name()));
        }
    }
}

/// Simple version-aware comparison (similar to strverscmp)
fn version_cmp(a: &str, b: &str) -> std::cmp::Ordering {
    let a_bytes = a.as_bytes();
    let b_bytes = b.as_bytes();
    let mut i = 0;
    let mut j = 0;

    while i < a_bytes.len() && j < b_bytes.len() {
        let ca = a_bytes[i];
        let cb = b_bytes[j];

        if ca.is_ascii_digit() && cb.is_ascii_digit() {
            let mut na: u64 = 0;
            while i < a_bytes.len() && a_bytes[i].is_ascii_digit() {
                na = na * 10 + (a_bytes[i] - b'0') as u64;
                i += 1;
            }
            let mut nb: u64 = 0;
            while j < b_bytes.len() && b_bytes[j].is_ascii_digit() {
                nb = nb * 10 + (b_bytes[j] - b'0') as u64;
                j += 1;
            }
            match na.cmp(&nb) {
                std::cmp::Ordering::Equal => continue,
                other => return other,
            }
        } else {
            match ca.cmp(&cb) {
                std::cmp::Ordering::Equal => {
                    i += 1;
                    j += 1;
                }
                other => return other,
            }
        }
    }
    a_bytes.len().cmp(&b_bytes.len())
}
