

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum DisplayMode {
    OnePerLine,
    InColumns,
    InRows,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum EscapeMode {
    None,
    Question,
    C,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum FlagStyle {
    None,
    Normal, // -F: / @ * | =
    Old,    // -O: [] @ *
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum SortType {
    ByName,
    ByTime,
    BySize,
    Unsorted,
    ByVersion,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TimeType {
    Mtime,
    Ctime,
    Atime,
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[allow(dead_code)]
pub enum TimeStyle {
    Traditional,
    Iso,
    Relative,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum SizeStyle {
    Default,
    Human,
}

#[derive(Debug, Clone, Copy, PartialEq)]
enum Tri {
    Default,
    Off,
    On,
}

pub struct Options {
    pub all: bool,
    pub blocksize: u64,
    pub bytes: bool,
    pub color: bool,
    pub datetime: bool,
    pub directory: bool,
    pub dirs_only: bool,
    pub dir_totals: bool,
    pub display_mode: DisplayMode,
    pub escape: EscapeMode,
    pub flags: FlagStyle,
    pub follow_dir_link_args: bool,
    pub group: bool,
    pub inode: bool,
    pub link_count: bool,
    pub modes: bool,
    pub numeric: bool,
    pub owner: bool,
    pub perms: bool,
    pub recursive: bool,
    pub reverse: bool,
    pub show_link: bool,
    pub show_links: bool,
    pub size: bool,
    pub size_style: SizeStyle,
    pub sort_type: SortType,
    pub target_info: bool,
    pub time_style: TimeStyle,
    pub time_type: TimeType,
    pub screen_width: u16,
    pub now: i64,
}

impl Options {
    pub fn new() -> Self {
        let is_tty = unsafe { libc::isatty(libc::STDOUT_FILENO) } != 0;
        let screen_width = get_terminal_width().unwrap_or(80);
        let blocksize = std::env::var("BLOCKSIZE")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(1024u64);

        Options {
            all: false,
            blocksize,
            bytes: false,
            color: false,
            datetime: false,
            directory: false,
            dirs_only: false,
            dir_totals: false,
            display_mode: if is_tty {
                DisplayMode::InColumns
            } else {
                DisplayMode::OnePerLine
            },
            escape: if is_tty {
                EscapeMode::Question
            } else {
                EscapeMode::None
            },
            flags: FlagStyle::None,
            follow_dir_link_args: true,
            group: false,
            inode: false,
            link_count: false,
            modes: false,
            numeric: false,
            owner: false,
            perms: false,
            recursive: false,
            reverse: false,
            show_link: false,
            show_links: false,
            size: false,
            size_style: SizeStyle::Default,
            sort_type: SortType::ByName,
            target_info: false,
            time_style: TimeStyle::Traditional,
            time_type: TimeType::Mtime,
            screen_width,
            now: now_time(),
        }
    }

    pub fn parse_args(&mut self, args: &[String]) -> Vec<String> {
        let mut file_args = Vec::new();
        let mut follow_dir_link_args = Tri::Default;
        let mut target_info = Tri::Default;
        let mut had_flags = false;
        let mut had_long = false;
        let mut compatible_sort = false;

        let mut i = 1; // skip program name
        while i < args.len() {
            let arg = &args[i];
            if !arg.starts_with('-') || arg == "-" {
                // not a flag, collect remaining as file args
                file_args.extend_from_slice(&args[i..]);
                break;
            }
            if arg == "--" {
                file_args.extend_from_slice(&args[i + 1..]);
                break;
            }

            let chars: Vec<char> = arg[1..].chars().collect();
            let mut j = 0;
            while j < chars.len() {
                match chars[j] {
                    '1' => self.display_mode = DisplayMode::OnePerLine,
                    'a' => self.all = true,
                    'B' | 'b' => self.bytes = true,
                    'C' => self.display_mode = DisplayMode::InColumns,
                    'c' => {
                        self.time_type = TimeType::Ctime;
                        compatible_sort = true;
                    }
                    'D' => self.dirs_only = true,
                    'd' => self.directory = true,
                    'E' => self.escape = EscapeMode::None,
                    'e' => self.escape = EscapeMode::C,
                    'F' => {
                        self.flags = FlagStyle::Normal;
                        had_flags = true;
                    }
                    'f' | 'U' => self.sort_type = SortType::Unsorted,
                    'G' | 'K' => self.color = true,
                    'g' => self.group = true,
                    'H' => follow_dir_link_args = Tri::On,
                    'h' => self.size_style = SizeStyle::Human,
                    'I' => self.time_style = TimeStyle::Iso,
                    'i' => self.inode = true,
                    'k' => self.blocksize = 1024,
                    'L' => {
                        target_info = Tri::On;
                        follow_dir_link_args = Tri::On;
                    }
                    'l' => {
                        self.modes = true;
                        self.link_count = true;
                        self.owner = true;
                        self.group = true;
                        self.bytes = true;
                        self.datetime = true;
                        self.show_link = true;
                        self.display_mode = DisplayMode::OnePerLine;
                        self.dir_totals = true;
                        had_long = true;
                    }
                    'M' | 'm' => self.modes = true,
                    'N' => self.link_count = true,
                    'n' => {
                        self.numeric = true;
                        // -n implies -l
                        self.modes = true;
                        self.link_count = true;
                        self.owner = true;
                        self.group = true;
                        self.bytes = true;
                        self.datetime = true;
                        self.show_link = true;
                        self.display_mode = DisplayMode::OnePerLine;
                        self.dir_totals = true;
                        had_long = true;
                    }
                    'O' => {
                        self.flags = FlagStyle::Old;
                        had_flags = true;
                    }
                    'o' => self.owner = true,
                    'P' => {
                        target_info = Tri::Off;
                        follow_dir_link_args = Tri::Off;
                    }
                    'p' => self.perms = true,
                    'q' => self.escape = EscapeMode::Question,
                    'R' => self.recursive = true,
                    'r' => self.reverse = true,
                    'S' => self.sort_type = SortType::BySize,
                    's' => {
                        self.size = true;
                        self.dir_totals = true;
                    }
                    'T' => {
                        self.datetime = true;
                        // Check for Tc, Tu suffixes
                        if j + 1 < chars.len() {
                            match chars[j + 1] {
                                'c' => {
                                    self.time_type = TimeType::Ctime;
                                    j += 1;
                                }
                                'u' => {
                                    self.time_type = TimeType::Atime;
                                    j += 1;
                                }
                                _ => {}
                            }
                        }
                    }
                    't' => {
                        self.sort_type = SortType::ByTime;
                        // Check for tc, tu suffixes
                        if j + 1 < chars.len() {
                            match chars[j + 1] {
                                'c' => {
                                    self.time_type = TimeType::Ctime;
                                    j += 1;
                                }
                                'u' => {
                                    self.time_type = TimeType::Atime;
                                    j += 1;
                                }
                                _ => {}
                            }
                        }
                    }
                    'u' => {
                        self.time_type = TimeType::Atime;
                        compatible_sort = true;
                    }
                    'V' => self.show_links = true,
                    'v' => self.sort_type = SortType::ByVersion,
                    'x' => self.display_mode = DisplayMode::InRows,
                    _ => {
                        eprintln!("l: unknown option: -{}", chars[j]);
                    }
                }
                j += 1;
            }
            i += 1;
        }

        // Handle -c/-u compatible sort: if neither -T nor -l were given,
        // these imply sort by time
        if compatible_sort && !self.datetime && !had_long {
            self.sort_type = SortType::ByTime;
        }

        // Handle follow_dir_link_args tri-state
        self.follow_dir_link_args = match follow_dir_link_args {
            Tri::On => true,
            Tri::Off => false,
            Tri::Default => {
                if had_flags || had_long {
                    false
                } else {
                    true
                }
            }
        };

        // Handle target_info
        match target_info {
            Tri::On => {
                self.target_info = true;
                self.show_link = false;
            }
            Tri::Off => {
                self.target_info = false;
            }
            Tri::Default => {}
        }

        file_args
    }
}

fn get_terminal_width() -> Option<u16> {
    unsafe {
        let mut ws: libc::winsize = std::mem::zeroed();
        if libc::ioctl(libc::STDOUT_FILENO, libc::TIOCGWINSZ, &mut ws) == 0 && ws.ws_col > 0 {
            Some(ws.ws_col)
        } else {
            std::env::var("COLUMNS")
                .ok()
                .and_then(|s| s.parse().ok())
        }
    }
}

fn now_time() -> i64 {
    unsafe {
        let mut t: libc::time_t = 0;
        libc::time(&mut t);
        t
    }
}
