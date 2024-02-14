#include <stddef.h>    // for size_t
#include <array>       // for array
#include <chrono>      // for operator""s, chrono_literals
#include <cmath>       // for sin
#include <functional>  // for ref, reference_wrapper, function
#include <memory>      // for allocator, shared_ptr, __shared_ptr_access
#include <string>  // for string, basic_string, char_traits, operator+, to_string
#include <thread>   // for sleep_for, thread
#include <utility>  // for move
#include <vector>   // for vector
#include <filesystem>
#include <fstream>
#include <limits> //  for numeric limits 
#include <ranges>
#include <algorithm>

#include "ftxui/component/component.hpp"  // for Checkbox, Renderer, Horizontal, Vertical, Input, Menu, Radiobox, ResizableSplitLeft, Tab
#include "ftxui/component/component_base.hpp"  // for ComponentBase, Component
#include "ftxui/component/component_options.hpp"  // for MenuOption, InputOption
#include "ftxui/component/event.hpp"              // for Event, Event::Custom
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, color, operator|, bgcolor, filler, Element, vbox, size, hbox, separator, flex, window, graph, EQUAL, paragraph, WIDTH, hcenter, Elements, bold, vscroll_indicator, HEIGHT, flexbox, hflow, border, frame, flex_grow, gauge, paragraphAlignCenter, paragraphAlignJustify, paragraphAlignLeft, paragraphAlignRight, dim, spinner, LESS_THAN, center, yframe, GREATER_THAN
#include "ftxui/dom/flexbox_config.hpp"  // for FlexboxConfig
#include "ftxui/screen/color.hpp"  // for Color, Color::BlueLight, Color::RedLight, Color::Black, Color::Blue, Color::Cyan, Color::CyanLight, Color::GrayDark, Color::GrayLight, Color::Green, Color::GreenLight, Color::Magenta, Color::MagentaLight, Color::Red, Color::White, Color::Yellow, Color::YellowLight, Color::Default, Color::Palette256, ftxui
#include "ftxui/screen/color_info.hpp"  // for ColorInfo
#include "ftxui/screen/terminal.hpp"    // for Size, Dimensions
#include <ftxui/dom/table.hpp>      // for Table, TableSelection

#include "refl.hpp"
#include "util.hpp"
#include "filesystem-tree.hpp"

struct TableElement {
    // Size value wil lbe stored. Name will be used for text 
    // with refl.cpp library
    std::string name;
    std::string type;
    std::string parent; 
    
    std::function<void()> on_click = [&]() {std::cout << "Clicked " << name << std::endl; };
};

REFL_TYPE(TableElement)
  REFL_FIELD(name)
  REFL_FIELD(type)
  REFL_FIELD(parent)
REFL_END

template<IsVectorOrArray T>
struct TableContainer{
    ftxui::Component tableContainer;
    using element_type = typename T::value_type;
    using filter_type  = std::function<bool(element_type)>;
    
    private:
    ftxui::Component tableContent;
    std::vector<size_t> columnSizes;
    ftxui::Element tableHeader;
    T &data;
    

    std::function<ftxui::ButtonOption(std::string name, std::function<void()> &on_click, int size)> buttonOption = [this](std::string name, std::function<void()> &on_click, int size) {
        auto option = ftxui::ButtonOption::Simple(); 
        /* auto option = ButtonOption::Border(); */
        // auto option = ButtonOption::Animated();
        option.on_click = on_click;
        option.label = name;
        option.transform = [size](const ftxui::EntryState& s) {
            auto element = ftxui::text(s.label) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, size);
            if (s.focused) {
            element |= ftxui::bold;
            element |= ftxui::blink;
            }
            return element;
        };
        return option;
    };

    public:
    std::function<void(filter_type)> updateTable = [this](filter_type filter){
        //TODO assert tableElement count and column sizes are the same
        tableContent->DetachAllChildren();

        for (auto el : this->data | std::ranges::views::filter(filter))
        {
            auto rowContainer = ftxui::Container::Horizontal({});

            int j=0;
            refl::util::for_each(refl::reflect(el).members, [&](auto member)
            { 
                auto msg(member(el));
                auto cellContainer = ftxui::Button(buttonOption( msg, el.on_click, this->columnSizes[j]));
                rowContainer->Add(cellContainer);
                j++;
            });

            tableContent->Add(rowContainer);
        }
    };

    TableContainer(T &data, std::vector<size_t> &columnSizes) :
    data{data}, columnSizes{columnSizes}
    {
        tableContent = ftxui::Container::Vertical({});

        // Generate tableHeader
        ftxui::Elements tableElements;
        size_t elCtr{0};
        refl::util::for_each(refl::reflect(data[0]).members, [&](auto member){
            //std::cout << member.name << " " <<member(tableData[0]) << std::endl;
            std::string name(member.name);
            tableElements.push_back( ftxui::text(name)  | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, columnSizes[elCtr]));
            elCtr++;
        });
        tableHeader = ftxui::hbox(tableElements);
        
        // Generate table entries from data
        updateTable([](const element_type& el){return true;});

        // Create Renderer
        tableContainer = ftxui::Renderer(tableContent, [&] {
            return ftxui::vbox({
                    tableHeader,
                    tableContent->Render() | ftxui::vscroll_indicator
                });
        });
    }
};

//----------------------
//
//----------------------
ftxui::Component MultilineText(ftxui::Elements e)
{
  class Impl : public ftxui::ComponentBase {
  public:
    explicit Impl(ftxui::Elements e) : elements_{std::move(e)} {
    }

    ftxui::Element Render() final {
      return ftxui::vbox(elements_);
    }

  private:
    ftxui::Elements elements_;
  };

  return ftxui::Make<Impl>(std::move(e));
}


// Take a list of component, display them vertically, one column shifted to the
// right.
ftxui::Component Inner(std::vector<ftxui::Component> children) {
  ftxui::Component vlist = ftxui::Container::Vertical(std::move(children));
  return ftxui::Renderer(vlist, [vlist] {
    return ftxui::hbox({
        ftxui::text(" "),
        vlist->Render(),
    });
  });
}

ftxui::Component forEachNodeGui(Node *node)
{
    if (!node)
        return std::make_shared<ftxui::ComponentBase>();

    if(node->isFile)
    {
        ftxui::Components childComponents;        

        auto contents = node->getFileContents(32);

        auto splitStream = [&](std::stringstream &ss){
            ftxui::Elements fileElements;
            std::vector<std::string> lines;
            std::string line;
            while(std::getline(ss, line, '\n'))
                lines.push_back(line);

            for(const auto& line : lines)
                fileElements.push_back( ftxui::text(line)  | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, line.size()));

            auto fileContentsComonent = MultilineText(fileElements);
            childComponents.push_back(fileContentsComonent);
        };
        splitStream(contents.first);
        splitStream(contents.second);
        return ftxui::Collapsible(node->name, ftxui::Container::Vertical(childComponents));
    }
    else
    {
        ftxui::Components childComponents;
        for (auto& child : node->children) {
            childComponents.push_back(forEachNodeGui(child.second));
        }
        return ftxui::Collapsible(node->name, Inner(childComponents));
    }
}
//----------------------
// Floating Message helper
//----------------------
struct FloatingMessage
{
    ftxui::Component component;
    std::vector<std::string> &msg;
    std::function<void()> hideModal;

    FloatingMessage(std::function<void()> hideModal, std::vector<std::string> &msg) :
    msg{msg}, hideModal{hideModal}
    {

        auto messageComponent = ftxui::Renderer( [&] {
            ftxui::Elements msgElements;
            for(auto it : msg)
                msgElements.push_back( ftxui::text(it) );
            return ftxui::vbox( std::move(msgElements) ) |
                ftxui::center;
        });
        component = ftxui::Container::Vertical({
            messageComponent,
            ftxui::Button("OK", hideModal, ftxui::ButtonOption::Border()) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL,10)| ftxui::center,
        });

        // Polish how the two buttons are rendered:
        component |= ftxui::Renderer([&](ftxui::Element inner) {
            return ftxui::vbox({ftxui::separator(), inner })
                | ftxui::size(ftxui::WIDTH,ftxui::GREATER_THAN, 50)
                | ftxui::border;
        });
    }
};

void DisplayMainUI( bool fullscreen)
{
    auto screen_fullscreen = ftxui::ScreenInteractive::Fullscreen();
    auto screen_fit = ftxui::ScreenInteractive::FitComponent();
    auto& screen = fullscreen ? screen_fullscreen : screen_fit;
    bool showHelp;

    //----------------------------------------------------------------------------
    // Test File-system
    //----------------------------------------------------------------------------

    // Provide the path to the parent folder
    std::string parentFolderPath = ".";

    // Provide the regex filters to include or exclude certain files or directories
    std::string includeRegex = ""; // Empty string means no filtering
    // std::string excludeRegex = "^(.*\\.out|\\.git|(zig-cache)|(zig-out)|.*\\.cpp)$";
    std::string excludeRegex = "^(.*\\.out|\\.git|(zig-cache)|(zig-out))$";
    // std::string excludeRegex = "";

    // Create FileSystemTree object with printContents option enabled, maximum bytes to read, maximum characters to show, include filter, and exclude filter
    FileSystemTree fileSystem(includeRegex, excludeRegex); // Will read only first 100 bytes of each file, show only first 50 characters of interpreted string content, and exclude files or directories matching the provided regex filter

    // Construct the file system tree from the given directory
    fileSystem.constructFromFilesystem(parentFolderPath);

    // Displaying the file system tree
    // fileSystem.displayFileSystem(true,32);

    auto componentCollapsible = forEachNodeGui(fileSystem.root);

    //----------------------------------------------------------------------------
    // Register table
    //----------------------------------------------------------------------------

    std::vector<size_t> columnWidth{10,20,20};
    std::vector<TableElement> tableData0{{"name0","0","name1"},{"name1","1","name0"},{"name2","2","name0"}}; 
    TableContainer inst0{tableData0, columnWidth};
    
    // ---------------------------------------------------------------------------
    // Tabs
    // ---------------------------------------------------------------------------
    constexpr int tabInstances = 2;
    std::array<ftxui::Component, tabInstances>   tabsContainer{ftxui::Container::Vertical({}), ftxui::Container::Vertical({})}; 
    int tabIndex = 0;
    std::vector<std::string> tabEntries = {"DSI-0", "DSI-1"};
    auto tabSelection = ftxui::Menu(&tabEntries, &tabIndex, ftxui::MenuOption::Horizontal());
    auto tabContent = ftxui::Container::Tab(
        {
            inst0.tableContainer,
            componentCollapsible,
        },
        &tabIndex);

    auto tabContainer = ftxui::Container::Vertical({
        tabSelection,
        tabContent,
    });

    auto tabRenderer = ftxui::Renderer(tabContainer, [&] {
      return ftxui::vbox({
          tabSelection->Render(),
          ftxui::separator(),
          tabContent->Render(),
      })| ftxui::border;
    });

    //----------------------------------------------------------------------------
    // Search bar
    //----------------------------------------------------------------------------
    std::string searchFilter;
    int selectedSearch = 0;
    ftxui::Component searchRenderer;
    ftxui::Component searchBar;
    std::function<void()> onSearchUpdate;
    ftxui::InputOption searchOptions;

    std::vector<std::string> searchEntries{};
    refl::util::for_each(refl::reflect(tableData0[0]).members, [&](auto member){
        searchEntries.push_back(member.name.str());
    });
    searchOptions= ftxui::InputOption::Default();
    searchOptions.multiline = false;
    searchOptions.on_enter = [&]{
        auto filter =[](const TableElement& el){return true;};

        if(searchFilter.length() < 1) inst0.updateTable(filter);
        else if(selectedSearch == 0) inst0.updateTable( [searchFilter](const TableElement& el){return el.name.find(searchFilter) != std::string::npos; } );
        else if(selectedSearch == 1) inst0.updateTable( [searchFilter](const TableElement& el){return el.type.find(searchFilter) != std::string::npos; } );
        else inst0.updateTable(filter);
    };

    searchBar = ftxui::Container::Horizontal({
                    ftxui::Radiobox(&searchEntries, &selectedSearch) ,
                    ftxui::Input(&searchFilter, "Search pattern", searchOptions)| ftxui::border
    });
    searchRenderer = ftxui::Renderer(searchBar, [&] {
    return ftxui::hbox({
               ftxui::text("Search: "),
               ftxui::separator(),
               searchBar->Render(),
           });
    });

    auto onUpdateTableEvent =[&]()
    {
        searchOptions.on_enter();
    };

    auto searchComponent = ftxui::Container::Horizontal({
        searchRenderer,
        ftxui::Button("Update Values", onUpdateTableEvent ,ftxui::ButtonOption::Simple()),
        ftxui::Button("Quit", screen.ExitLoopClosure(),ftxui::ButtonOption::Simple()),
    }); 


    //------------------------------------------------------------
    // Main component
    //------------------------------------------------------------
    auto mainComponent = ftxui::Container::Vertical({
        searchComponent,
        tabRenderer,
    }) | ftxui::border;

    auto mainRenderer = ftxui::Renderer(mainComponent, [&] {
    return ftxui::hbox({
               mainComponent->Render(),
           });
    });
    
    //------------------------------------------------------------
    // Wrap it inside a frame, to allow scrolling.
    //------------------------------------------------------------
    mainRenderer = ftxui::Renderer(mainRenderer, [mainRenderer] { return mainRenderer->Render() | ftxui::yframe; });

    ftxui::Event previous_event;
    ftxui::Event next_event;
    auto wrappedComponent = CatchEvent(mainRenderer, [&](ftxui::Event event)
    {
        previous_event = next_event;
        next_event = event;

        // --------------------------------------
        if (event == ftxui::Event::F1) {
            showHelp = true;   
            return true;
        }

        // Allow the user to quit using ESC -----
        if (event == ftxui::Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    //----------------------------------------------------------------------------
    // Help dialog
    //----------------------------------------------------------------------------
    std::vector<std::string> helpMessage;
    helpMessage.push_back(std::string{"test 1"});
    helpMessage.push_back(std::string{"test 2"});
    helpMessage.push_back(std::string{"test 3"});
    auto hideHelp = [&] { showHelp = false; };

    FloatingMessage helpModal{hideHelp, helpMessage};
    wrappedComponent |= ftxui::Modal(helpModal.component, &showHelp);
    // No mouse interaction
    screen.TrackMouse(false);
    screen.Loop(wrappedComponent);
}
