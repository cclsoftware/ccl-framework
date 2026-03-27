#######
Testing
#######

Unit tests are an important tool in software development and serve multiple purposes:

* **Bug Detection**: Identify bugs early during development
* **Regression Testing**: Ensure that existing functionality remains intact during refactoring or as new features are added
* **Documentation**: Document the requirements and constraints to which the code under test is implemented
* **Software Design**: Encourage modular design with low coupling and high cohesion by creating small, isolated units that can be effectively tested
* **Confidence**: Unit tests provide developers with confidence and peace of mind that their code works as intended

Check out chapter :ref:`"How do I add a unit test?"<ccl_add_unit_test>` to get started with unit tests.

.. _unit_test_policy:

======
Policy
======

Unit tests should follow the F.I.R.S.T acronym:

* **Fast**: Tests should provide instant feedback
* **Isolated**: Tests must not depend on the execution of other tests
* **Repeatable**: Multiple execution should yield the same results
* **Small**: Readers should be able to determine what's tested and what has failed at a glance without looking anywhere else
* **Timely**: Tests should be written before or alongside the code that is tested

Add unit tests for

* Non-trivial code (algorithms, math functions, ...)
* Units prone to hard-to-find bugs during regular use (parsers, components with user input, ...)
* Framework units and fundamental building blocks (Core/CCL components, ...)
* Regression bugs

Don't add unit tests for

* High level code, acting as integration tests
* Code that can't be automatically tested (e.g. graphical UI code)

======
Naming
======

* Test suite names have a "Test" suffix

.. code-block:: cpp

  // right
  CCL_TEST (BicycleTest, BicyclesHaveTwoWheels)
  {
	  // ...
  }

  // wrong
  CCL_TEST (Bicycle, BicyclesHaveTwoWheels)
  {
	  // ...
  }

* Test names describe what is tested and the expected result

.. code-block:: cpp

  // right
  CCL_TEST (BicycleTest, BicyclesHaveTwoWheels)
  {
	  // ...
  }

  // wrong
  CCL_TEST (BicycleTest, WheelTest)
  {
	  // ...
  }

=========
Structure
=========

* Use fixtures where multiple tests use the same initialization

.. code-block:: cpp

	// right

	//************************************************************************************************
	// BicycleTest
	//************************************************************************************************

	class BicycleTest: public Test
	{
	protected:
		Bicycle bicycle;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, BicyclesHaveTwoWheels)
	{
		int wheelCount = bicycle.getWheelCount ();
		CCL_TEST_ASSERT_EQUAL (2, wheelCount);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, BicyclesHaveABell)
	{
		CCL_TEST_ASSERT (bicycle.hasBell ());
	}

	// wrong
	
	//************************************************************************************************
	// BicycleTest
	//************************************************************************************************

	CCL_TEST (BicycleTest, BicyclesHaveTwoWheels)
	{
		Bicycle bicycle;
		int wheelCount = bicycle.getWheelCount ();
		CCL_TEST_ASSERT_EQUAL (2, wheelCount);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST (BicycleTest, BicyclesHaveABell)
	{
		Bicycle bicycle;
		CCL_TEST_ASSERT (bicycle.hasBell ());
	}

* Don't extract the actual test
* Don't use test assertions outside the test body, test set-up or test tear-down methods

.. code-block:: cpp

	// right

	//************************************************************************************************
	// BicycleTest
	//************************************************************************************************
	
	class BicycleTest: public Test
	{
	protected:
		Bicycle bicycle;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, FrontWheelIsRemoved)
	{
		bicycle.removeWheel (Bicycle::kFrontWheel);
		CCL_TEST_ASSERT_EQUAL (1, bicycle.getWheelCount ());
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, RearWheelIsRemoved)
	{
		bicycle.removeWheel (Bicycle::kRearWheel);
		CCL_TEST_ASSERT_EQUAL (1, bicycle.getWheelCount ());
	}

	// wrong

	//************************************************************************************************
	// BicycleTest
	//************************************************************************************************
	
	class BicycleTest: public Test
	{
	protected:
		Bicycle bicycle;

		void testRemoveWheel (int index)
		{
			bicycle.removeWheel (index);
			CCL_TEST_ASSERT_EQUAL (1, bicycle.getWheelCount ());
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, FrontWheelIsRemoved)
	{
		testRemoveWheel (0);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, RearWheelIsRemoved)
	{
		testRemoveWheel (1);
	}

=============
Good Practice
=============

* Follow the **AAA** pattern:

#. **Arrange**: Put system into the desired initial state
#. **Act**: Call a *single* function to test
#. **Assert**: Verify system is in the expected state after **Act**

.. code-block:: cpp

	// example
	CCL_TEST (BicycleTest, OpposingWindMakesCyclingMoreExhausting)
	{
		// Arrange
		Environment environment;
		Rider rider;
		Bicycle bicycle (&rider, &environment);
		bicycle.setSpeed (20);
		bicycle.setDirection (Direction::kWest);
		int exhaustmentBefore = rider.getExhaustmentLevel ();

		// Act
		environent.setWind (Direction::kEast, 10);

		// Assert
		CCL_TEST_ASSERT (rider.getExhaustmentLevel () > exhaustmentBefore);
	}

* If the arrange block or part of the arrangement is shared among multiple tests, it should be placed in the test fixture's "setUp ()" method
* The arrangement or scenario must be described well enough by the test fixture name in order to understand it

.. code-block:: cpp

	//************************************************************************************************
	// BicycleTest
	//************************************************************************************************

	class BicycleTest: public Test
	{
	public:
		// Test
		void setUp () override
		{
			// Arrange (shared part)
			bicycle = NEW Bicycle (&rider, &environment);
		}

	protected:
		Environment environment;
		Rider rider;
		AutoPtr<Bicycle> bicycle;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	CCL_TEST_F (BicycleTest, OpposingWindMakesCyclingMoreExhausting)
	{
		// Arrange (individual part)
		bicycle.setSpeed (20);
		bicycle.setDirection (Direction::kWest);
		int exhaustmentBefore = rider.getExhaustmentLevel ();

		// Act
		environent.setWind (Direction::kEast, 10);

		// Assert
		CCL_TEST_ASSERT (rider.getExhaustmentLevel () > exhaustmentBefore);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	CCL_TEST_F (BicycleTest, SomeOtherTest)
	{
		// ...
	}

* Avoid trivial tests

.. code-block:: cpp

	// don't test
	CCL_TEST (BicycleTest, BicycleIsConstructed)
	{
		AutoPtr<Bicycle> bicycle (NEW Bicycle ());
		CCL_TEST_ASSERT (bicycle != nullptr);
	}

Source files
============

* file names end on *test* (*stringtest.cpp*, *vectortest.cpp*, ...)