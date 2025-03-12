#ifndef HTML_GENERATOR_H
#define HTML_GENERATOR_H

#include <Arduino.h>
#include "thermostat_state.h"
#include "interfaces/config_interface.h"
#include "interfaces/control_interface.h"

class HtmlGenerator {
public:
    static String generatePage(
        ThermostatState* state,
        ConfigInterface* config,
        ControlInterface* control = nullptr,
        const String& csrfToken = ""
    );

private:
    static String generateHeader(const String& csrfToken = "");
    static String generateNavigation();
    static String generateStatusSection(ThermostatState* state);
    static String generateControlSection(ThermostatState* state, const String& csrfToken = "");
    static String generateConfigSection(ConfigInterface* config, const String& csrfToken = "");
    static String generatePIDSection(ControlInterface* control, const String& csrfToken = "");
    static String generateFooter();
    static String generateStyles();
    static String generateScripts();
};

#endif // HTML_GENERATOR_H