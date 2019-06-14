package main

import (
	"fmt"
	"reflect"
)

type Person struct {
	name string
	age  int
}

type People struct {
	iris Person
	cato Person
}

func writeTo(to, from reflect.Value) error {
	to = reflect.Indirect(to)
	if to.Type() == from.Type() {
		to.Set(from)
		return nil
	}
	switch to.Kind() {
	case reflect.Ptr:
		iface := to.Interface()
		switch v := iface.(type) {
		case *int:
			fmt.Println("INT!! %T", v)
		}

		fmt.Println("E: %v", iface)
		// return writeTo(to, from)
	}
	return fmt.Errorf("cannot map!! type %s to %s", from.Type().Name(), to.Kind())
}

func readTo(to interface{}, from interface{}) error {
	return writeTo(reflect.ValueOf(to), reflect.ValueOf(from))

	switch v := to.(type) {
	case *interface{}:
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
		source, ok := from.(int)
		if ok {
			dest := to.(*int)
			*dest = source
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
		source, ok := from.(string)
		if ok {
			dest := to.(*string)
			*dest = source
			break
		}
		return fmt.Errorf("cannot map type %T to %T", from, v)
	case **string:
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
		fmt.Println("BLA")
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
					fmt.Println("HERE!")
					vv := e.Field(i).UnsafeAddr()
					// vv.UnsafeAddr
					if err := readTo(vv, val); err != nil {
						fmt.Println("ERR HERE!")
						return err
					}
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
	var p *int

	// source := map[string]interface{}{
	// 	"name": "Iris!",
	// 	"age":  6,
	// }

	err := readTo(&p, 5)
	if err != nil {
		fmt.Println(err)
	}

	fmt.Printf("Value: %v\n", p)
}
