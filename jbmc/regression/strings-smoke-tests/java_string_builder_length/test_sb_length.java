public class test_sb_length
{
   public static void main()
   {
      StringBuilder x = new StringBuilder("abc");
      x.append("de");
      assert(x.length() == 5);
   }
}
