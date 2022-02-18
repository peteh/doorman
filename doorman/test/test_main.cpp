#include <unity.h>
#include <Arduino.h>
#include <TriggerPatternRecognition.h>


void test_GivenPattern_WhenPatternSuccessful_ThenTrigger()
{
    // arrange
    TriggerPatternRecognition recognition;
    recognition.addStep(2000);
    recognition.addStep(3000);
    
    // act
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(2000);
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(3000);
    
    // assert
    TEST_ASSERT_TRUE(recognition.trigger());
}

void test_Given1s1s1sPattern_WhenPatternSuccessful_ThenTrigger()
{
    // arrange
    TriggerPatternRecognition recognition;
    recognition.addStep(1000);
    recognition.addStep(1000);
    recognition.addStep(1000);
    
    // act
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(1000);
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(1000);
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(1000);
    
    // assert
    TEST_ASSERT_TRUE(recognition.trigger());
}


void test_Given2s3sPattern_WhenPatternWrong_ThenDontTrigger()
{
    // arrange
    TriggerPatternRecognition recognition;
    recognition.addStep(2000);
    recognition.addStep(3000);
    
    // act
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(2000);
    TEST_ASSERT_FALSE(recognition.trigger());
    delay(2000);
    
    // assert
    TEST_ASSERT_FALSE(recognition.trigger());
}

void setup()
{
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_GivenPattern_WhenPatternSuccessful_ThenTrigger);
    RUN_TEST(test_Given1s1s1sPattern_WhenPatternSuccessful_ThenTrigger);
    RUN_TEST(test_Given2s3sPattern_WhenPatternWrong_ThenDontTrigger);
    // RUN_TEST(test_function_calculator_multiplication);
    // RUN_TEST(test_function_calculator_division);
    UNITY_END();
}

void loop()
{
    delay(1000);
}
