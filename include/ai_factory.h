/**
 * @file ai_factory.h
 * @author tdecabrat
 * @brief This file defines the AI factory, which is responsible for creating
 * AI instances based on a given name and configuration parameters. It provides
 * a method to create an AI by name, which can be used to initialize different
 * AI strategies such as Minimax, MCTS, or Random.
 */
#ifndef AI_FACTORY_H
#define AI_FACTORY_H

#include "ai_interface.h"

AI_Interface create_ai (const char *name, ConfigMap params);

#endif