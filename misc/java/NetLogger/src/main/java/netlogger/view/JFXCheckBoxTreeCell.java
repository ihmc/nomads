package netlogger.view;

import com.jfoenix.controls.JFXCheckBox;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.beans.value.ObservableValue;
import javafx.collections.ObservableList;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.util.Callback;
import javafx.util.StringConverter;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/23/2018
 */
public class JFXCheckBoxTreeCell<T> extends TreeCell<T>
{
/***************************************************************************
 *                                                                         *
 * Static cell factories                                                   *
 *                                                                         *
 **************************************************************************/

    /**
     * Creates a cell factory for use in a TreeView control, although there is a
     * major assumption when used in a TreeView: this cell factory assumes that
     * the TreeView root, and <b>all</b> children are instances of
     * {@link CheckBoxTreeItem}, rather than the default {@link TreeItem} class
     * that is used normally.
     *
     * <p>When used in a TreeView, the CheckBoxCell is rendered with a CheckBox
     * to the right of the 'disclosure node' (i.e. the arrow). The item stored
     * in {@link CheckBoxTreeItem#getValue()} will then have the StringConverter
     * called on it, and this text will take all remaining horizontal space.
     * Additionally, by using {@link CheckBoxTreeItem}, the TreeView will
     * automatically handle situations such as:
     *
     * <ul>
     *   <li>Clicking on the {@link CheckBox} beside an item that has children
     *      will result in all children also becoming selected/unselected.</li>
     *   <li>Clicking on the {@link CheckBox} beside an item that has a parent
     *      will possibly toggle the state of the parent. For example, if you
     *      select a single child, the parent will become indeterminate (indicating
     *      partial selection of children). If you proceed to select all
     *      children, the parent will then show that it too is selected. This is
     *      recursive, with all parent nodes updating as expected.</li>
     * </ul>
     *
     * <p>Unfortunately, due to limitations in Java, it is necessary to provide
     * an explicit cast when using this method. For example:
     *
     * <pre>
     * {@code
     * final TreeView<String> treeView = new TreeView<String>();
     * treeView.setCellFactory(CheckBoxCell.<String>forTreeView());}</pre>
     *
     * @param <T> The type of the elements contained within the
     *      {@link CheckBoxTreeItem} instances.
     * @return A {@link Callback} that will return a TreeCell that is able to
     *      work on the type of element contained within the TreeView root, and
     *      all of its children (recursively).
     */
    public static <T> Callback<TreeView<T>, TreeCell<T>> forTreeView() {
        Callback<TreeItem<T>, ObservableValue<Boolean>> getSelectedProperty =
                item -> {
                    if (item instanceof CheckBoxTreeItem<?>) {
                        return ((CheckBoxTreeItem<?>)item).selectedProperty();
                    }
                    return null;
                };
        return forTreeView(getSelectedProperty,
                CellUtils.<T>defaultTreeItemStringConverter());
    }

    /**
     * Creates a cell factory for use in a TreeView control. Unlike
     * {@link #forTreeView()}, this method does not assume that all TreeItem
     * instances in the TreeView are {@link CheckBoxTreeItem} instances.
     *
     * <p>When used in a TreeView, the CheckBoxCell is rendered with a CheckBox
     * to the right of the 'disclosure node' (i.e. the arrow). The item stored
     * in {@link CheckBoxTreeItem#getValue()} will then have the StringConverter
     * called on it, and this text will take all remaining horizontal space.
     *
     * <p>Unlike {@link #forTreeView()}, this cell factory does not handle
     * updating the state of parent or children TreeItems - it simply toggles
     * the {@code ObservableValue<Boolean>} that is provided, and no more. Of
     * course, this functionality can then be implemented externally by adding
     * observers to the {@code ObservableValue<Boolean>}, and toggling the state
     * of other properties as necessary.
     *
     * @param <T> The type of the elements contained within the {@link TreeItem}
     *      instances.
     * @param getSelectedProperty A {@link Callback} that, given an object of
     *      type TreeItem<T>, will return an {@code ObservableValue<Boolean>}
     *      that represents whether the given item is selected or not. This
     *      {@code ObservableValue<Boolean>} will be bound bidirectionally
     *      (meaning that the CheckBox in the cell will set/unset this property
     *      based on user interactions, and the CheckBox will reflect the state
     *      of the {@code ObservableValue<Boolean>}, if it changes externally).
     * @return A {@link Callback} that will return a TreeCell that is able to
     *      work on the type of element contained within the TreeView root, and
     *      all of its children (recursively).
     */
    public static <T> Callback<TreeView<T>, TreeCell<T>> forTreeView(
            final Callback<TreeItem<T>,
                    ObservableValue<Boolean>> getSelectedProperty) {
        return forTreeView(getSelectedProperty, CellUtils.<T>defaultTreeItemStringConverter());
    }

    /**
     * Creates a cell factory for use in a TreeView control. Unlike
     * {@link #forTreeView()}, this method does not assume that all TreeItem
     * instances in the TreeView are {@link CheckBoxTreeItem}.
     *
     * <p>When used in a TreeView, the CheckBoxCell is rendered with a CheckBox
     * to the right of the 'disclosure node' (i.e. the arrow). The item stored
     * in {@link TreeItem#getValue()} will then have the the StringConverter
     * called on it, and this text will take all remaining horizontal space.
     *
     * <p>Unlike {@link #forTreeView()}, this cell factory does not handle
     * updating the state of parent or children TreeItems - it simply toggles
     * the {@code ObservableValue<Boolean>} that is provided, and no more. Of
     * course, this functionality can then be implemented externally by adding
     * observers to the {@code ObservableValue<Boolean>}, and toggling the state
     * of other properties as necessary.
     *
     * @param <T> The type of the elements contained within the {@link TreeItem}
     *      instances.
     * @param getSelectedProperty A Callback that, given an object of
     *      type TreeItem<T>, will return an {@code ObservableValue<Boolean>}
     *      that represents whether the given item is selected or not. This
     *      {@code ObservableValue<Boolean>} will be bound bidirectionally
     *      (meaning that the CheckBox in the cell will set/unset this property
     *      based on user interactions, and the CheckBox will reflect the state of
     *      the {@code ObservableValue<Boolean>}, if it changes externally).
     * @param converter A StringConverter that, give an object of type TreeItem<T>,
     *      will return a String that can be used to represent the object
     *      visually. The default implementation in {@link #forTreeView(Callback)}
     *      is to simply call .toString() on all non-null items (and to just
     *      return an empty string in cases where the given item is null).
     * @return A {@link Callback} that will return a TreeCell that is able to
     *      work on the type of element contained within the TreeView root, and
     *      all of its children (recursively).
     */
    public static <T> Callback<TreeView<T>, TreeCell<T>> forTreeView(
            final Callback<TreeItem<T>, ObservableValue<Boolean>> getSelectedProperty,
            final StringConverter<TreeItem<T>> converter) {
        return tree -> new JFXCheckBoxTreeCell<>(getSelectedProperty, converter);
    }




    /***************************************************************************
     *                                                                         *
     * Fields                                                                  *
     *                                                                         *
     **************************************************************************/
    private final JFXCheckBox checkBox;

    private ObservableValue<Boolean> booleanProperty;

    private BooleanProperty indeterminateProperty;



    /***************************************************************************
     *                                                                         *
     * Constructors                                                            *
     *                                                                         *
     **************************************************************************/

    /**
     * Creates a default {@link JFXCheckBoxTreeCell} that assumes the TreeView is
     * constructed with {@link CheckBoxTreeItem} instances, rather than the
     * default {@link TreeItem}.
     * By using {@link CheckBoxTreeItem}, it will internally manage the selected
     * and indeterminate state of each item in the tree.
     */
    public JFXCheckBoxTreeCell() {
        // getSelectedProperty as anonymous inner class to deal with situation
        // where the user is using CheckBoxTreeItem instances in their tree
        this(item -> {
            if (item instanceof CheckBoxTreeItem<?>) {
                return ((CheckBoxTreeItem<?>)item).selectedProperty();
            }
            return null;
        });
    }

    /**
     * Creates a {@link JFXCheckBoxTreeCell} for use in a TreeView control via a
     * cell factory. Unlike {@link JFXCheckBoxTreeCell#JFXCheckBoxTreeCell()}, this
     * method does not assume that all TreeItem instances in the TreeView are
     * {@link CheckBoxTreeItem}.
     *
     * <p>To call this method, it is necessary to provide a
     * {@link Callback} that, given an object of type TreeItem<T>, will return
     * an {@code ObservableValue<Boolean>} that represents whether the given
     * item is selected or not. This {@code ObservableValue<Boolean>} will be
     * bound bidirectionally (meaning that the CheckBox in the cell will
     * set/unset this property based on user interactions, and the CheckBox will
     * reflect the state of the {@code ObservableValue<Boolean>}, if it changes
     * externally).
     *
     * <p>If the items are not {@link CheckBoxTreeItem} instances, it becomes
     * the developers responsibility to handle updating the state of parent and
     * children TreeItems. This means that, given a TreeItem, this class will
     * simply toggles the {@code ObservableValue<Boolean>} that is provided, and
     * no more. Of course, this functionality can then be implemented externally
     * by adding observers to the {@code ObservableValue<Boolean>}, and toggling
     * the state of other properties as necessary.
     *
     * @param getSelectedProperty A {@link Callback} that will return an
     *      {@code ObservableValue<Boolean>} that represents whether the given
     *      item is selected or not.
     */
    public JFXCheckBoxTreeCell(
            final Callback<TreeItem<T>, ObservableValue<Boolean>> getSelectedProperty) {
        this(getSelectedProperty, CellUtils.<T>defaultTreeItemStringConverter(), null);
    }

    /**
     * Creates a {@link JFXCheckBoxTreeCell} for use in a TreeView control via a
     * cell factory. Unlike {@link JFXCheckBoxTreeCell#JFXCheckBoxTreeCell()}, this
     * method does not assume that all TreeItem instances in the TreeView are
     * {@link CheckBoxTreeItem}.
     *
     * <p>To call this method, it is necessary to provide a {@link Callback}
     * that, given an object of type TreeItem<T>, will return an
     * {@code ObservableValue<Boolean>} that represents whether the given item
     * is selected or not. This {@code ObservableValue<Boolean>} will be bound
     * bidirectionally (meaning that the CheckBox in the cell will set/unset
     * this property based on user interactions, and the CheckBox will reflect
     * the state of the {@code ObservableValue<Boolean>}, if it changes
     * externally).
     *
     * <p>If the items are not {@link CheckBoxTreeItem} instances, it becomes
     * the developers responsibility to handle updating the state of parent and
     * children TreeItems. This means that, given a TreeItem, this class will
     * simply toggles the {@code ObservableValue<Boolean>} that is provided, and
     * no more. Of course, this functionality can then be implemented externally
     * by adding observers to the {@code ObservableValue<Boolean>}, and toggling
     * the state of other properties as necessary.
     *
     * @param getSelectedProperty A {@link Callback} that will return an
     *      {@code ObservableValue<Boolean>} that represents whether the given
     *      item is selected or not.
     * @param converter A StringConverter that, give an object of type TreeItem<T>, will
     *      return a String that can be used to represent the object visually.
     */
    public JFXCheckBoxTreeCell(
            final Callback<TreeItem<T>, ObservableValue<Boolean>> getSelectedProperty,
            final StringConverter<TreeItem<T>> converter) {
        this(getSelectedProperty, converter, null);
    }

    private JFXCheckBoxTreeCell(
            final Callback<TreeItem<T>, ObservableValue<Boolean>> getSelectedProperty,
            final StringConverter<TreeItem<T>> converter,
            final Callback<TreeItem<T>, ObservableValue<Boolean>> getIndeterminateProperty) {
        this.getStyleClass().add("check-box-tree-cell");
        setSelectedStateCallback(getSelectedProperty);
        setConverter(converter);

        this.checkBox = new JFXCheckBox();
        this.checkBox.setAllowIndeterminate(false);

        // by default the graphic is null until the cell stops being empty
        setGraphic(null);
    }



    /***************************************************************************
     *                                                                         *
     * Properties                                                              *
     *                                                                         *
     **************************************************************************/

    // --- converter
    private ObjectProperty<StringConverter<TreeItem<T>>> converter =
            new SimpleObjectProperty<StringConverter<TreeItem<T>>>(this, "converter");

    /**
     * The {@link StringConverter} property.
     */
    public final ObjectProperty<StringConverter<TreeItem<T>>> converterProperty() {
        return converter;
    }

    /**
     * Sets the {@link StringConverter} to be used in this cell.
     */
    public final void setConverter(StringConverter<TreeItem<T>> value) {
        converterProperty().set(value);
    }

    /**
     * Returns the {@link StringConverter} used in this cell.
     */
    public final StringConverter<TreeItem<T>> getConverter() {
        return converterProperty().get();
    }



    // --- selected state callback property
    private ObjectProperty<Callback<TreeItem<T>, ObservableValue<Boolean>>>
            selectedStateCallback =
            new SimpleObjectProperty<Callback<TreeItem<T>, ObservableValue<Boolean>>>(
                    this, "selectedStateCallback");

    /**
     * Property representing the {@link Callback} that is bound to by the
     * CheckBox shown on screen.
     */
    public final ObjectProperty<Callback<TreeItem<T>, ObservableValue<Boolean>>> selectedStateCallbackProperty() {
        return selectedStateCallback;
    }

    /**
     * Sets the {@link Callback} that is bound to by the CheckBox shown on screen.
     */
    public final void setSelectedStateCallback(Callback<TreeItem<T>, ObservableValue<Boolean>> value) {
        selectedStateCallbackProperty().set(value);
    }

    /**
     * Returns the {@link Callback} that is bound to by the CheckBox shown on screen.
     */
    public final Callback<TreeItem<T>, ObservableValue<Boolean>> getSelectedStateCallback() {
        return selectedStateCallbackProperty().get();
    }



    /***************************************************************************
     *                                                                         *
     * Public API                                                              *
     *                                                                         *
     **************************************************************************/

    /** {@inheritDoc} */
    @Override public void updateItem(T item, boolean empty) {
        super.updateItem(item, empty);

        if (empty) {
            setText(null);
            setGraphic(null);
        } else {
            StringConverter<TreeItem<T>> c = getConverter();

            TreeItem<T> treeItem = getTreeItem();

            // update the node content
            setText(c != null ? c.toString(treeItem) : (treeItem == null ? "" : treeItem.toString()));
            checkBox.setGraphic(treeItem == null ? null : treeItem.getGraphic());

            HBox graphic = new HBox(checkBox);
            graphic.setPadding(new Insets(0,0,0,7));
            setGraphic(graphic);

            // uninstall bindings
            if (booleanProperty != null) {
                checkBox.selectedProperty().unbindBidirectional((BooleanProperty)booleanProperty);
            }
            if (indeterminateProperty != null) {
                checkBox.indeterminateProperty().unbindBidirectional(indeterminateProperty);
            }

            // install new bindings.
            // We special case things when the TreeItem is a CheckBoxTreeItem
            if (treeItem instanceof CheckBoxTreeItem) {
                CheckBoxTreeItem<T> cbti = (CheckBoxTreeItem<T>) treeItem;
                booleanProperty = cbti.selectedProperty();
                checkBox.selectedProperty().bindBidirectional((BooleanProperty)booleanProperty);

                indeterminateProperty = cbti.indeterminateProperty();
                checkBox.indeterminateProperty().bindBidirectional(indeterminateProperty);
            } else {
                Callback<TreeItem<T>, ObservableValue<Boolean>> callback = getSelectedStateCallback();
                if (callback == null) {
                    throw new NullPointerException(
                            "The CheckBoxTreeCell selectedStateCallbackProperty can not be null");
                }

                booleanProperty = callback.call(treeItem);
                if (booleanProperty != null) {
                    checkBox.selectedProperty().bindBidirectional((BooleanProperty)booleanProperty);
                }
            }
        }
    }

    private static class CellUtils
    {
        static int TREE_VIEW_HBOX_GRAPHIC_PADDING = 3;

        /***************************************************************************
         *                                                                         *
         * Private fields                                                          *
         *                                                                         *
         **************************************************************************/

        private final static StringConverter<?> defaultStringConverter = new StringConverter<Object>()
        {
            @Override
            public String toString (Object t) {
                return t == null ? null : t.toString();
            }

            @Override
            public Object fromString (String string) {
                return (Object) string;
            }
        };

        private final static StringConverter<?> defaultTreeItemStringConverter =
                new StringConverter<TreeItem<?>>()
                {
                    @Override
                    public String toString (TreeItem<?> treeItem) {
                        return (treeItem == null || treeItem.getValue() == null) ?
                                "" : treeItem.getValue().toString();
                    }

                    @Override
                    public TreeItem<?> fromString (String string) {
                        return new TreeItem<>(string);
                    }
                };

        /***************************************************************************
         *                                                                         *
         * General convenience                                                     *
         *                                                                         *
         **************************************************************************/

        /*
         * Simple method to provide a StringConverter implementation in various cell
         * implementations.
         */
        @SuppressWarnings("unchecked")
        static <T> StringConverter<T> defaultStringConverter () {
            return (StringConverter<T>) defaultStringConverter;
        }

        /*
         * Simple method to provide a TreeItem-specific StringConverter
         * implementation in various cell implementations.
         */
        @SuppressWarnings("unchecked")
        static <T> StringConverter<TreeItem<T>> defaultTreeItemStringConverter () {
            return (StringConverter<TreeItem<T>>) defaultTreeItemStringConverter;
        }

        private static <T> String getItemText (Cell<T> cell, StringConverter<T> converter) {
            return converter == null ?
                    cell.getItem() == null ? "" : cell.getItem().toString() :
                    converter.toString(cell.getItem());
        }


        static Node getGraphic (TreeItem<?> treeItem) {
            return treeItem == null ? null : treeItem.getGraphic();
        }


        /***************************************************************************
         *                                                                         *
         * ChoiceBox convenience                                                   *
         *                                                                         *
         **************************************************************************/

        static <T> void updateItem (final Cell<T> cell,
                                    final StringConverter<T> converter,
                                    final ChoiceBox<T> choiceBox) {
            updateItem(cell, converter, null, null, choiceBox);
        }

        static <T> void updateItem (final Cell<T> cell,
                                    final StringConverter<T> converter,
                                    final HBox hbox,
                                    final Node graphic,
                                    final ChoiceBox<T> choiceBox) {
            if (cell.isEmpty()) {
                cell.setText(null);
                cell.setGraphic(null);
            }
            else {
                if (cell.isEditing()) {
                    if (choiceBox != null) {
                        choiceBox.getSelectionModel().select(cell.getItem());
                    }
                    cell.setText(null);

                    if (graphic != null) {
                        hbox.getChildren().setAll(graphic, choiceBox);
                        cell.setGraphic(hbox);
                    }
                    else {
                        cell.setGraphic(choiceBox);
                    }
                }
                else {
                    cell.setText(getItemText(cell, converter));
                    cell.setGraphic(graphic);
                }
            }
        }

        ;

        static <T> ChoiceBox<T> createChoiceBox (
                final Cell<T> cell,
                final ObservableList<T> items,
                final ObjectProperty<StringConverter<T>> converter) {
            ChoiceBox<T> choiceBox = new ChoiceBox<T>(items);
            choiceBox.setMaxWidth(Double.MAX_VALUE);
            choiceBox.converterProperty().bind(converter);
            choiceBox.getSelectionModel().selectedItemProperty().addListener((ov, oldValue, newValue) -> {
                if (cell.isEditing()) {
                    cell.commitEdit(newValue);
                }
            });
            return choiceBox;
        }


        /***************************************************************************
         *                                                                         *
         * TextField convenience                                                   *
         *                                                                         *
         **************************************************************************/

        static <T> void updateItem (final Cell<T> cell,
                                    final StringConverter<T> converter,
                                    final TextField textField) {
            updateItem(cell, converter, null, null, textField);
        }

        static <T> void updateItem (final Cell<T> cell,
                                    final StringConverter<T> converter,
                                    final HBox hbox,
                                    final Node graphic,
                                    final TextField textField) {
            if (cell.isEmpty()) {
                cell.setText(null);
                cell.setGraphic(null);
            }
            else {
                if (cell.isEditing()) {
                    if (textField != null) {
                        textField.setText(getItemText(cell, converter));
                    }
                    cell.setText(null);

                    if (graphic != null) {
                        hbox.getChildren().setAll(graphic, textField);
                        cell.setGraphic(hbox);
                    }
                    else {
                        cell.setGraphic(textField);
                    }
                }
                else {
                    cell.setText(getItemText(cell, converter));
                    cell.setGraphic(graphic);
                }
            }
        }

        static <T> void startEdit (final Cell<T> cell,
                                   final StringConverter<T> converter,
                                   final HBox hbox,
                                   final Node graphic,
                                   final TextField textField) {
            if (textField != null) {
                textField.setText(getItemText(cell, converter));
            }
            cell.setText(null);

            if (graphic != null) {
                hbox.getChildren().setAll(graphic, textField);
                cell.setGraphic(hbox);
            }
            else {
                cell.setGraphic(textField);
            }

            textField.selectAll();

            // requesting focus so that key input can immediately go into the
            // TextField (see RT-28132)
            textField.requestFocus();
        }

        static <T> void cancelEdit (Cell<T> cell, final StringConverter<T> converter, Node graphic) {
            cell.setText(getItemText(cell, converter));
            cell.setGraphic(graphic);
        }

        static <T> TextField createTextField (final Cell<T> cell, final StringConverter<T> converter) {
            final TextField textField = new TextField(getItemText(cell, converter));

            // Use onAction here rather than onKeyReleased (with check for Enter),
            // as otherwise we encounter RT-34685
            textField.setOnAction(event -> {
                if (converter == null) {
                    throw new IllegalStateException(
                            "Attempting to convert text input into Object, but provided "
                                    + "StringConverter is null. Be sure to set a StringConverter "
                                    + "in your cell factory.");
                }
                cell.commitEdit(converter.fromString(textField.getText()));
                event.consume();
            });
            textField.setOnKeyReleased(t -> {
                if (t.getCode() == KeyCode.ESCAPE) {
                    cell.cancelEdit();
                    t.consume();
                }
            });
            return textField;
        }


        /***************************************************************************
         *                                                                         *
         * ComboBox convenience                                                   *
         *                                                                         *
         **************************************************************************/

        static <T> void updateItem (Cell<T> cell, StringConverter<T> converter, ComboBox<T> comboBox) {
            updateItem(cell, converter, null, null, comboBox);
        }

        static <T> void updateItem (final Cell<T> cell,
                                    final StringConverter<T> converter,
                                    final HBox hbox,
                                    final Node graphic,
                                    final ComboBox<T> comboBox) {
            if (cell.isEmpty()) {
                cell.setText(null);
                cell.setGraphic(null);
            }
            else {
                if (cell.isEditing()) {
                    if (comboBox != null) {
                        comboBox.getSelectionModel().select(cell.getItem());
                    }
                    cell.setText(null);

                    if (graphic != null) {
                        hbox.getChildren().setAll(graphic, comboBox);
                        cell.setGraphic(hbox);
                    }
                    else {
                        cell.setGraphic(comboBox);
                    }
                }
                else {
                    cell.setText(getItemText(cell, converter));
                    cell.setGraphic(graphic);
                }
            }
        }

        ;

        static <T> ComboBox<T> createComboBox (final Cell<T> cell,
                                               final ObservableList<T> items,
                                               final ObjectProperty<StringConverter<T>> converter) {
            ComboBox<T> comboBox = new ComboBox<T>(items);
            comboBox.converterProperty().bind(converter);
            comboBox.setMaxWidth(Double.MAX_VALUE);
            comboBox.getSelectionModel().selectedItemProperty().addListener((ov, oldValue, newValue) -> {
                if (cell.isEditing()) {
                    cell.commitEdit(newValue);
                }
            });
            return comboBox;
        }
    }
}