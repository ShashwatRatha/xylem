use ignore::Walk;
use tree_sitter::{Parser, Query, QueryCursor};

mod get_captures;

fn main() {
    let mut c_parser = Parser::new();
    let language = tree_sitter_c::LANGUAGE.into();

    c_parser
        .set_language(&language)
        .expect("Failed loading C lang");

    let query_str = r#"
(function_definition
    (storage_class_specifier) @fn.storage
    (#eq? @fn.storage "static")
    declarator: [
        (function_declarator
            declarator: (identifier) @fn.name)
        (pointer_declarator
            declarator: (function_declarator
            declarator: (identifier) @fn.name))
    ]
) @fn.is_static

(function_definition
    (storage_class_specifier)? @fn.storage
    (#not-eq? @fn.storage "static")
    declarator: [
        (function_declarator
            declarator: (identifier) @fn.name)
        (pointer_declarator
            declarator: (function_declarator
            declarator: (identifier) @fn.name))
    ]
) @fn.is_non_static
"#;

    match Query::new(&language, query_str) {
        Ok(query) => {
            let mut cursor = QueryCursor::new();

            for entry in Walk::new(".").filter_map(|e| e.ok()) {
                if let filename = entry.path().display().to_string().as_str()
                    && filename.ends_with(".c")
                {
                    // println!("{name}:\n==================================");
                    let src = std::fs::read_to_string(filename).expect("Error opening C file");
                    let tree = c_parser
                        .parse(&src, None)
                        .expect("Error parsing the C code");

                    get_captures::print_function_map(&src, &tree, &mut cursor, &query, &filename);
                }
            }
        },
        Err(e) => {
            println!("{}", e.message);
        }
    }
}
