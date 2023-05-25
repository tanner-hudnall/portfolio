def canConstruct(self, ransomNote, magazine):
        note, mag = Counter(ransomNote), Counter(magazine)
        
        if note & mag == note:
            return True
        else:
            return False
          
# Explanation: Counter objects sort these strings into dictionaries with 'keys' of characters and 'values' of their frequencies, if we use the 
# & bitwise operator, we are able to find the intersection of the two objects, which should be the count of the common characters with the minimum 
# frequency between the two strings. If this result is equivalent to the ransom note, then we know it is in the magazine and we can return True, if
# not, we can return false
          
# Problem Description:
# Given two strings ransomNote and magazine, return true if ransomNote can be constructed by using the letters from magazine and false otherwise.
# Each letter in magazine can only be used once in ransomNote.

# Example 1:
# Input: ransomNote = "a", magazine = "b"
# Output: false

# Example 2:
# Input: ransomNote = "aa", magazine = "ab"
# Output: false

# Example 3:
# Input: ransomNote = "aa", magazine = "aba"
# Output: true
 
# Constraints:
# 1 <= ransomNote.length, magazine.length <= 105
# ransomNote and magazine consist of lowercase English letters.
