#include "CommandRouter.h"
#include "OtaGuard.h"

namespace fy {

bool CommandRouter_enqueue(BringupContext* context,
                           const CommandMessage& command, bool priority) {
    if (context == nullptr) return false;
    // OTA ist eine atomare, kernuebergreifende Sperre.
    // Physischer/HTTP STOP 55 AA bleibt ueber die Prioritaetsqueue moeglich.
    if (OtaGuard_active() &&
        !(priority && command.id == CMD_STOPP &&
          command.data[0] == 0x55U && command.data[1] == 0xAAU))
        return false;
    QueueHandle_t queue = priority ? context->priorityCommands : context->commands;
    if (queue == nullptr) return false;
    // Die Motor-Task hat nur einen Eigentumer; STOP besitzt seine eigene Queue.
    return xQueueSend(queue, &command, 0) == pdTRUE;
}

} // namespace fy
