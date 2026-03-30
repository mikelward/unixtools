/// Terminal color support using ANSI escape codes
#[derive(Clone)]
#[allow(dead_code)]
pub struct Colors {
    pub red: &'static str,
    pub green: &'static str,
    pub blue: &'static str,
    pub cyan: &'static str,
    pub none: &'static str,
}

impl Colors {
    pub fn new() -> Option<Self> {
        // Check that TERM is set (basic terminal check)
        let term = std::env::var("TERM").unwrap_or_default();
        if term.is_empty() || term == "dumb" {
            return None;
        }

        Some(Colors {
            red: "\x1b[31m",
            green: "\x1b[32m",
            blue: "\x1b[34m",
            cyan: "\x1b[36m",
            none: "\x1b[0m",
        })
    }
}
