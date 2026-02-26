#pragma once

/**
 * @file module.h
 */

#include "../nlohmann/json.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "core/style.h"
#include "imgui/imgui_flags.h"

#include "common/dsp/buffer.h" // TODOREWORK ditch

#define UITO_C_STR(input) "%s", std::to_string(input).c_str() // TODOREWORK?
#define REGISTER_MODULE(module) modules_registry.push_back(satdump::pipeline::ModuleEntry{module::getID(), module::getParams(), module::getInstance})
#define REGISTER_MODULE_EXTERNAL(registry, module) registry.push_back(satdump::pipeline::ModuleEntry{module::getID(), module::getParams(), module::getInstance})

namespace satdump
{
    namespace pipeline
    {
        enum ModuleDataType
        {
            DATA_FILE,        // Just generic data from a file
            DATA_STREAM,      // Generic data, for which a circular buffer will be used
            DATA_DSP_STREAM,  // DSP Data using the specialized buffer and complex floats
            DATA_NDSP_STREAM, // TODOREWORK
        };

        /**
         * @brief Core SatDump Pipeline module class
         *
         * A pipeline is composed of several modules each running
         * one after the other. Each module needs to take input data,
         * process it, and pass it on to the next.
         *
         * Modules can always process from a file, sometimes take an input
         * from a FIFO or DSP Stream, and sometimes also output in one of
         * those formats as well to allow them to run in realtime.
         *
         * Modules should be used in the following way :
         * - ProcessingModule() : Call the constructor
         * - setInputType() / setOutputType() : Set input/output types
         * - init() : Prepares some things so process() can run
         * - process() : Run the actual processing
         * - stop() : Optional, if running from a stream, this must be
         * called to make process() exit                   TODOREWORK maybe use a FIFO marker to stop it?
         * - getOutput() : If the output type was a file, get its path
         * to pass it on to the next module in the pipeline
         *
         * getOutput may return nothing for modules meant as final pipeline
         * steps.
         */
        class ProcessingModule
        {
        protected:                       // TODOREWORK document
            nlohmann::json d_parameters; // TODOREWORK enforce const again?

            const std::string d_input_file;
            const std::string d_output_file_hint;
            std::string d_output_file;

            ModuleDataType input_data_type, output_data_type;

            bool d_is_streaming_input;

        public:
            /**
             * @brief Constructor.
             *
             * @param input_file the input file to process (doesn't matter if later setting up for streaming input) TODOREWORK?
             * @param output_file_hint output file path hint (most modules just add an extension to it) TODOREWORK?
             * @param parameters module settings TODOREWORK?
             */
            ProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);

        public:
            /**
             * @brief Get supported input types for this module
             *
             * @return list of supported inputs
             */
            virtual std::vector<ModuleDataType> getInputTypes() { return {DATA_FILE}; }

            /**
             * @brief Get supported output types for this module
             *
             * @return list of supported outputs
             */
            virtual std::vector<ModuleDataType> getOutputTypes() { return {DATA_FILE}; }

            /**
             * @brief Set input type to use
             *
             * @param type the type
             */
            void setInputType(ModuleDataType type);

            /**
             * @brief Set output type to use
             *
             * @param type the type
             */
            void setOutputType(ModuleDataType type);

            /**
             * @brief Get input type in use
             *
             * @return the type
             */
            ModuleDataType getInputType();

            /**
             * @brief Get output type in use
             *
             * @return the type
             */
            ModuleDataType getOutputType();

            /**
             * @brief If available, provides the name of the
             * produced output file
             *
             * @return path of the generated file
             */
            std::string getOutput();

        public:
            /**
             * @brief Initialize the module, making it ready to call
             * process().
             * This must be called after setting input/output types,
             * and before calling process()
             */
            virtual void init();

            /**
             * @brief If running from a streaming input, this must be called
             * to make process() quit.
             *
             * TODOREWORK maybe use a FIFO marker to stop it?
             */
            virtual void stop();

            /**
             * @brief Implements actual processing.
             * Usually, this will include an internal loop reading from a
             * file/fifo, doing some stuff, and writing it out again.
             *
             * Once this exits, getOutput should be usable.
             */
            virtual void process() = 0;

            /**
             * @brief Draw the module's GUI, show live stats of what it's
             * currently doing.
             *
             * @param window set to true to render the module in its own ImGui Window
             */
            virtual void drawUI(bool window) = 0;

        public:
            // TODOREWORK document
            std::shared_ptr<dsp::RingBuffer<uint8_t>> input_fifo;
            std::shared_ptr<dsp::RingBuffer<uint8_t>> output_fifo;
            std::shared_ptr<dsp::stream<complex_t>> input_stream;
            std::shared_ptr<dsp::stream<complex_t>> output_stream;
            std::atomic<bool> input_active;

        public:
            /**
             * @brief Get the module's stats.
             *
             * Stats must be a list of JSON values representing various stats you
             * wish to make available. For example, this can be "synced" : true
             * or similar.
             *
             * @return JSON stats
             */
            virtual nlohmann::json getModuleStats() { return {}; }

        public:
            //     static std::string getID();
            virtual std::string getIDM() = 0;
            //     static nlohmann::json getParams() = 0;
            //     static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };

        /**
         * @brief Module registry entry
         *
         * @param id module string ID
         * @param params available module parameters and their defaults
         * @param inst function to create an instance of this module
         */
        struct ModuleEntry
        {
            std::string id;
            nlohmann::json params;
            std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, nlohmann::json)> inst;
        };

        /**
         * @brief Actual module registry
         */
        SATDUMP_DLL extern std::vector<ModuleEntry> modules_registry;

        // Event where modules are registered, so plugins can load theirs TODOREWORK
        struct RegisterModulesEvent
        {
            std::vector<ModuleEntry> &modules_registry;
        };

        /**
         * @brief Setup module registry. This must be called once at initialization.
         */
        void registerModules();

        /**
         * @brief Get an instance of a module by ID
         *
         * @return new module instance
         */
        std::shared_ptr<ProcessingModule> getModuleInstance(std::string id, std::string input_file, std::string output_file_hint, nlohmann::json parameters);

        /**
         * @brief Check if a module exists
         *
         * @return true if it exists
         */
        bool moduleExists(std::string id);
    } // namespace pipeline
} // namespace satdump