## Changelog \[v2025r1.2] – 2025-08-03

### License & Branding

* **Switched license** from MIT to **Business Source License 1.1 (BUSL-1.1)** This is to ensure the project remains freely available to all end-users, while protecting CrossBasic's future. Entities wishing to modify and use CrossBasic or any of its components for commercial purposes, must invest in CrossBasic by obtaining a commercial license - permitting more time, energy, and resources to be poured into CrossBasic's development. End-users are always permitted to sell products they have made using compiled releases of CrossBasic.


### API & Struct Changes

* **`Param` struct**: added a `bool isAssigns` field (default `false`) to support assignment semantics
* **`VM` struct**: introduced an `extensionMethods` map for module-extension support

  ````cpp
    std::unordered_map<std::string,
        std::unordered_map<std::string, Value>> extensionMethods;
  ````

### Lexer & Tokenizer Enhancements

* **New token types** in `XTokenType`:

  * `COLON`, `GOTO`, `EXTENDS`
  * `PLUS_EQUAL (+=)`, `MINUS_EQUAL (-=)`, `STAR_EQUAL (*=)`, `SLASH_EQUAL (/=)`
  * `ASSIGNS`

* **`Lexer.scanToken()`** updated to recognize `':'` and the new compound-assignment operators

### Plugin & Callback Improvements

* **Added**

  ```cpp
  static Value wrapHandleIfPluginClass(const std::string &raw,
                                       const std::string &typeName,
                                       VM &vm);
  ```

  – auto-converts numeric string handles into plugin instances based on declared types (** Including Plugin-based Classes - ie. Dim myButton as New XButton)
* **`invokeScriptCallback()`** enhanced to:

  * Wrap string parameters via `wrapHandleIfPluginClass`
  * Auto-State Referencing - Restore VM stack depth after callback execution
  
### CrossBasic 'Runs-Anywhere' IDE

* Backend server and compiler have been integrated with the IDE for building and running compiled desktop applications from the web-based IDE.
  - Support for all GUI objects showing in the IDE will be added as each is developed. Currently only XButton, XTextField, XTextArea, and XTimer are wired to the IDE compiler; The IDE currently builds crossbasic code for all controls that will be supported using "Build". Direct code creation of other existing UI plugin elements is supported in the code-editor or another text-based IDE of your choosing.

### Miscellaneous

* Standardized operator-matching logic and code styling refinements throughout the lexer and VM core

* Plugin standardization and initial creation of a few cross-platform GUI objects. Working instances of a majority of cross-platform controls exist within the XGUI examples and plugin, but are being refactored into OOP standalone control class objects.
