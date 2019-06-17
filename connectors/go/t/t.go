package main

import (
	"fmt"
	"reflect"
)

type Person struct {
	Name string `ti:"name"`
	Age  int    `ti:"age"`
}

type People struct {
	Iris Person
	Cato Person
}

func writeTo(to, from reflect.Value) error {
	// iFrom := reflect.ValueOf(*from)
	// indirectTo := reflect.Indirect(to)
	println("!!!1")
	if to.Type() == from.Type() {
		println("!!!2")
		to.Set(from)
		return nil
	}

	switch to.Kind() {
	case reflect.Struct:
		println("!!!13")
		// iface := to.Interface()
		// switch iface.(type) {
		m, ok := from.(map[string]interface{})
		if !ok {
			return fmt.Errorf("cannot map type %T to %T", from, v)
		}
		// 	t := e.Type()
		// 	n := t.NumField()

		// 	for i := 0; i < n; i++ {
		// 		field := t.Field(i)
		// 		fn := field.Tag.Get("ti")
		// 		if len(fn) == 0 {
		// 			fn = field.Name
		// 		}
		// 		val, ok := m[fn]
		// 		if ok {
		// 			err := writeTo(e.Field(i), reflect.ValueOf(val))
		// 			if err != nil {
		// 				println("!!!6")
		// 				return err
		// 			}
		// 		}
		// 	}
	}
	return fmt.Errorf("cannot map!! type %s to %s", from.Type().Name(), to.Kind())
}

func mapStruct(from interface{}) error {
	m, ok := from.(map[string]interface{})
	if !ok {
		return fmt.Errorf("cannot map type %T to %T", from, v)
	}
	t := e.Type()
	n := t.NumField()

	for i := 0; i < n; i++ {
		field := t.Field(i)
		fn := field.Tag.Get("ti")
		if len(fn) == 0 {
			fn = field.Name
		}
		val, ok := m[fn]
		if ok {
			// vv.UnsafeAddr
			// if err := readTo(vv, val); err != nil {
			// e.Field(i).Set(reflect.ValueOf(val))

			err := writeTo(e.Field(i), reflect.ValueOf(val))
			if err != nil {
				println("!!!6")
				return err
			}
			// 	return err
			// }
			// if e.Field(i).Type() != reflect.TypeOf(val) {
			// 	err := fmt.Errorf("unexpected type")
			// 	return err
			// }
			// e.Field(i).Set(reflect.ValueOf(x))
		}
	}
}

func readTo(to interface{}, from interface{}) error {
	// return writeTo(reflect.ValueOf(to), reflect.ValueOf(from))

	switch v := to.(type) {
	case *interface{}:
		println("!!!12")
		dest := to.(*interface{})
		*dest = from
	case *bool:
		source, ok := from.(bool)
		if ok {
			dest := to.(*bool)
			*dest = source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	case *int:
		println("!!!8")
		source, ok := from.(int)
		if ok {
			dest := to.(*int)
			*dest = source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	case **int:
		println("!!!7")
		if from == nil {
			dest := to.(**int)
			*dest = nil
			break
		}
		source, ok := from.(int)
		if ok {
			dest := to.(**int)
			*dest = &source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	case *float64:
		source, ok := from.(float64)
		if ok {
			dest := to.(*float64)
			*dest = source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	case *string:
		println("!!!9")
		source, ok := from.(string)
		if ok {
			dest := to.(*string)
			*dest = source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	case **string:
		println("!!!10")
		if from == nil {
			dest := to.(**string)
			*dest = nil
			break
		}
		source, ok := from.(string)
		if ok {
			dest := to.(**string)
			*dest = &source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	default:
		println("!!!5")
		e := reflect.ValueOf(to).Elem()
		switch e.Kind() {
		case reflect.Struct:
			m, ok := from.(map[string]interface{})
			if !ok {
				return fmt.Errorf("cannot map type %T to %T", from, v)
			}
			t := e.Type()
			n := t.NumField()

			for i := 0; i < n; i++ {
				field := t.Field(i)
				fn := field.Tag.Get("ti")
				if len(fn) == 0 {
					fn = field.Name
				}
				val, ok := m[fn]
				if ok {
					// vv.UnsafeAddr
					// if err := readTo(vv, val); err != nil {
					// e.Field(i).Set(reflect.ValueOf(val))

					err := writeTo(e.Field(i), reflect.ValueOf(val))
					if err != nil {
						println("!!!6")
						return err
					}
					// 	return err
					// }
					// if e.Field(i).Type() != reflect.TypeOf(val) {
					// 	err := fmt.Errorf("unexpected type")
					// 	return err
					// }
					// e.Field(i).Set(reflect.ValueOf(x))
				}
			}
		default:
			return fmt.Errorf("unsupported type %T", v)
		}
	}
	return nil
}

func main() {
	fmt.Println("Hello, playground")
	var p People

	source := map[string]interface{}{
		"Iris": map[string]interface{}{
			"name": "Iris!",
			"age":  6,
		},
		"Cato": map[string]interface{}{
			"name": "Cato!",
			"age":  6,
		},
	}

	err := readTo(&p, source)
	if err != nil {
		fmt.Println(err)
	}
	fmt.Printf("Value: %v\n", p)

}
