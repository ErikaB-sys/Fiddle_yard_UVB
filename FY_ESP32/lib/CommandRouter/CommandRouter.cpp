#include "CommandRouter.h"

namespace fy {

bool CommandRouter_enqueue(BringupContext* context,
                           const CommandMessage& command, bool priority) {
    if (context == nullptr) return false;
    QueueHandle_t queue = priority ? context->priorityCommands : context->commands;
    if (queue == nullptr) return false;
    // Die Motor-Task hat nur einen Eigentumer; STOP besitzt seine eigene Queue.
    return xQueueSend(queue, &command, 0) == pdTRUE;
}

} // namespace fy
