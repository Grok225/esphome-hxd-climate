import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, globals
from esphome.const import CONF_ID

AUTO_LOAD = ["climate"]
DEPENDENCIES = ["uart", "globals"]

# 定义命名空间和类
hxd_ac_ns = cg.esphome_ns.namespace("hxd_ac")
HXD_AC_Climate = hxd_ac_ns.class_("HXD_AC_Climate", climate.Climate, cg.Component)

# ✅ 正确暴露 CONFIG_SCHEMA —— 这是关键！
CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HXD_AC_Climate),
        cv.Required("uart_id"): cv.use_id(uart.UARTComponent),
        cv.Required("high_byte_id"): cv.use_id(globals.GlobalsComponent),
        cv.Required("low_byte_id"): cv.use_id(globals.GlobalsComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    uart_component = await cg.get_variable(config["uart_id"])
    high_byte_global = await cg.get_variable(config["high_byte_id"])
    low_byte_global = await cg.get_variable(config["low_byte_id"])

    cg.add(var.set_uart_bus(uart_component))
    cg.add(var.set_high_byte_global(high_byte_global))
    cg.add(var.set_low_byte_global(low_byte_global))
