#include <iostream>
#include <string>
#include <vector>

// signals header
#include "itomsignal.h"

/*
* Dummy class extending the Disconnector class - see the ITSignals.h
*/
class Widget : public itom::Disconnector
{
public:

    Widget(const std::string& name) :
        name_{ name }
    {
        // notify when the widget gains a focus
        focus_in_.Connect([this]() {
            std::cout << name_  << " widget: focus received\n";
        });
    }

    std::string GetName()
    {
        return name_;
    }

    void PrintName() const
    {
        std::cout << name_ << "\n";
    }

    // simple focus signal without a param
    itom::Signal<> focus_in_;

protected:

    std::string name_;

};

/*
* Another dummy class higher in the hierarchy
*/
class ListBox : public Widget
{
public:

    ListBox(const std::string& name) :
        Widget(name)
    {
        // notify what item changed its text and to what value
        item_text_changed_.Connect([this](int index, const std::string& text) {
            std::cout << name_ << " listbox: item on index " << index
                << " changed its text to \"" << text << "\"\n";
        });

        // notify what items are now selected in the multi-select listbox
        selection_changed_.Connect([this](const std::vector<int>& selected_items) {
            std::cout << name_ << " listbox: selected items are:\n";
            for (const auto& i : selected_items)
            {
                std::cout << i << "\n";
            }
        });
    }

    // signal for an item's text change - index and a new text of this item
    itom::Signal<int, std::string> item_text_changed_;

    // signal for a selection change - index vector of the selected items
    itom::Signal<std::vector<int>> selection_changed_;
};

/*
* Dummy class not extending the Disconnector class
*/
class NotDisconnector
{

};

int main()
{
    // create some dummy objects
    ListBox* listbox = new ListBox("MyListBox");
    Widget* widget = new Widget("SomeWidget");
    NotDisconnector* not_disconnector = new NotDisconnector();

    // this would not compile - Connect() expects class inheriting
    // from the Disconnector class
    //listbox->focus_in_.Connect(
    //    &Widget::PrintName,
    //    not_disconnector // w's dtor makes this slot to be disconnected
    //);

    // test the disconnection upon w's destruction
    // with non-static member function pointer
    listbox->focus_in_.Connect(
        &Widget::PrintName,
        widget // w's dtor makes this slot to be disconnected
    );

    // with lambda
    listbox->focus_in_.Connect([widget]() {
        std::cout << widget->GetName() << " non-member\n"; },
        widget // w's dtor makes this slot to be disconnected
    );

    // itom::Disconnector's virtual dtor terminates the above created connection
    delete widget;
    delete not_disconnector;

    size_t discon_slot_id = listbox->focus_in_.Connect([]() {
        std::cout << "don't print this\n";
    });
    
    // explicitly terminate a connection
    listbox->focus_in_.Disconnect(discon_slot_id);

    // multiple connections to the same signal
    listbox->focus_in_.Connect([]() {
        std::cout << "multiple slot test passed\n";
    });

    // this should execute 2 slots - 4 were connected
    // 1 was disconnected automatically (by Disconnector's dtor)
    // 1 was disconnected explicitly using Signal::Disconnect()
    listbox->focus_in_.Emit(); 

    // test the multi-param signal one of which is an lvalue
    constexpr int index = 3;
    listbox->item_text_changed_.Emit(index, "NewItem3");

    // test the container-param signal
    listbox->selection_changed_.Emit(std::vector<int>{ 0, 2, 99 });

    delete listbox;

    std::cin.get();
}
